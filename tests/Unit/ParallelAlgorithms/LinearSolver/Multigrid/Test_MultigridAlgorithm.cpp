// Distributed under the MIT License.
// See LICENSE.txt for details.

#define CATCH_CONFIG_RUNNER

#include <vector>

#include "Domain/Creators/DomainCreator.hpp"
#include "Domain/Creators/Interval.hpp"
#include "Domain/Creators/RegisterDerivedWithCharm.hpp"
#include "Elliptic/DiscontinuousGalerkin/DgElementArray.hpp"
#include "Helpers/Domain/BoundaryConditions/BoundaryCondition.hpp"
#include "Helpers/ParallelAlgorithms/LinearSolver/LinearSolverAlgorithmTestHelpers.hpp"
#include "Helpers/ParallelAlgorithms/LinearSolver/Multigrid/Helpers.hpp"
#include "IO/Observer/ObserverComponent.hpp"
#include "Options/Protocols/FactoryCreation.hpp"
#include "Parallel/Actions/Goto.hpp"
#include "Parallel/Actions/SetupDataBox.hpp"
#include "Parallel/InitializationFunctions.hpp"
#include "Parallel/Main.hpp"
#include "ParallelAlgorithms/Initialization/Actions/RemoveOptionsAndTerminatePhase.hpp"
#include "ParallelAlgorithms/LinearSolver/Multigrid/ElementsAllocator.hpp"
#include "ParallelAlgorithms/LinearSolver/Multigrid/Multigrid.hpp"
#include "ParallelAlgorithms/LinearSolver/Multigrid/Tags.hpp"
#include "ParallelAlgorithms/LinearSolver/Richardson/Richardson.hpp"
#include "Utilities/ErrorHandling/FloatingPointExceptions.hpp"
#include "Utilities/MemoryHelpers.hpp"
#include "Utilities/ProtocolHelpers.hpp"
#include "Utilities/TMPL.hpp"

// \cond
namespace PUP {
class er;
}  // namespace PUP
// \endcond

namespace helpers = LinearSolverAlgorithmTestHelpers;
namespace helpers_mg = TestHelpers::LinearSolver::multigrid;

namespace {

struct MultigridSolver {
  static constexpr Options::String help =
      "Options for the iterative linear solver";
};

struct RichardsonSmoother {
  static constexpr Options::String help =
      "Options for the iterative linear solver";
};

struct Metavariables {
  static constexpr const char* const help{
      "Test the Multigrid linear solver algorithm on multiple elements"};

  static constexpr size_t volume_dim = 1;
  using system =
      TestHelpers::domain::BoundaryConditions::SystemWithoutBoundaryConditions<
          volume_dim>;

  // [setup_smoother]
  using multigrid = LinearSolver::multigrid::Multigrid<
      volume_dim, helpers_mg::fields_tag, MultigridSolver,
      helpers_mg::OperatorIsMassive, helpers_mg::sources_tag>;

  using smoother = LinearSolver::Richardson::Richardson<
      typename multigrid::smooth_fields_tag, RichardsonSmoother,
      typename multigrid::smooth_source_tag,
      LinearSolver::multigrid::Tags::MultigridLevel>;
  // [setup_smoother]

  struct factory_creation
      : tt::ConformsTo<Options::protocols::FactoryCreation> {
    using factory_classes = tmpl::map<
        tmpl::pair<DomainCreator<1>, tmpl::list<domain::creators::Interval>>>;
  };

  using Phase = helpers::Phase;

  using initialization_actions =
      tmpl::list<::Actions::SetupDataBox, helpers_mg::InitializeElement,
                 typename multigrid::initialize_element,
                 typename smoother::initialize_element,
                 ::Initialization::Actions::RemoveOptionsAndTerminatePhase>;

  using register_actions = tmpl::list<typename multigrid::register_element,
                                      typename smoother::register_element,
                                      Parallel::Actions::TerminatePhase>;

  // [action_list]
  template <typename Label>
  using smooth_actions = tmpl::list<
      helpers_mg::ComputeOperatorAction<typename smoother::fields_tag>,
      typename smoother::template solve<
          helpers_mg::ComputeOperatorAction<typename smoother::operand_tag>,
          Label>>;

  using solve_actions =
      tmpl::list<typename multigrid::template solve<
                     smooth_actions<LinearSolver::multigrid::VcycleDownLabel>,
                     smooth_actions<LinearSolver::multigrid::VcycleUpLabel>>,
                 Parallel::Actions::TerminatePhase>;
  // [action_list]

  using test_actions =
      tmpl::list<helpers_mg::TestResult<typename multigrid::options_group>>;

  using component_list = tmpl::flatten<tmpl::list<
      typename multigrid::component_list, typename smoother::component_list,
      elliptic::DgElementArray<
          Metavariables,
          tmpl::list<
              Parallel::PhaseActions<Phase, Phase::Initialization,
                                     initialization_actions>,
              Parallel::PhaseActions<Phase, Phase::RegisterWithObserver,
                                     register_actions>,
              Parallel::PhaseActions<Phase, Phase::PerformLinearSolve,
                                     solve_actions>,
              Parallel::PhaseActions<Phase, Phase::TestResult, test_actions>>,
          LinearSolver::multigrid::ElementsAllocator<1, MultigridSolver>>,
      observers::Observer<Metavariables>,
      observers::ObserverWriter<Metavariables>,
      helpers::OutputCleaner<Metavariables>>>;
  using observed_reduction_data_tags =
      observers::collect_reduction_data_tags<tmpl::list<multigrid, smoother>>;
  static constexpr bool ignore_unrecognized_command_line_options = false;
  static constexpr auto determine_next_phase =
      helpers::determine_next_phase<Metavariables>;

  // NOLINTNEXTLINE(google-runtime-references)
  void pup(PUP::er& /*p*/) noexcept {}
};

}  // namespace

static const std::vector<void (*)()> charm_init_node_funcs{
    &setup_error_handling, &setup_memory_allocation_failure_reporting,
    &domain::creators::register_derived_with_charm};
static const std::vector<void (*)()> charm_init_proc_funcs{
    &enable_floating_point_exceptions};

using charmxx_main_component = Parallel::Main<Metavariables>;

#include "Parallel/CharmMain.tpp"  // IWYU pragma: keep