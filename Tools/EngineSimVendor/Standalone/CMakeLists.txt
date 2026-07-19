cmake_minimum_required(VERSION 3.20)
project(NC003BPhase0Standalone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(NEXTCAR_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(ENGINE_SIM_ROOT "${NEXTCAR_ROOT}/Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/engine-sim")
set(SOLVER_ROOT "${NEXTCAR_ROOT}/Plugins/NextcarEngineSim/Source/ThirdParty/EngineSim/upstream/simple-2d-constraint-solver")
set(CORE_ROOT "${NEXTCAR_ROOT}/Plugins/NextcarEngineSim/Source/NextcarEngineSimCore")

set(SOLVER_SOURCES
    ${SOLVER_ROOT}/src/clutch_constraint.cpp
    ${SOLVER_ROOT}/src/constraint.cpp
    ${SOLVER_ROOT}/src/fixed_position_constraint.cpp
    ${SOLVER_ROOT}/src/force_generator.cpp
    ${SOLVER_ROOT}/src/gauss_seidel_sle_solver.cpp
    ${SOLVER_ROOT}/src/line_constraint.cpp
    ${SOLVER_ROOT}/src/link_constraint.cpp
    ${SOLVER_ROOT}/src/matrix.cpp
    ${SOLVER_ROOT}/src/nsv_ode_solver.cpp
    ${SOLVER_ROOT}/src/ode_solver.cpp
    ${SOLVER_ROOT}/src/optimized_nsv_rigid_body_system.cpp
    ${SOLVER_ROOT}/src/rigid_body.cpp
    ${SOLVER_ROOT}/src/rigid_body_system.cpp
    ${SOLVER_ROOT}/src/rotation_friction_constraint.cpp
    ${SOLVER_ROOT}/src/sle_solver.cpp
    ${SOLVER_ROOT}/src/sparse_matrix.cpp
    ${SOLVER_ROOT}/src/system_state.cpp
    ${SOLVER_ROOT}/src/utilities.cpp
)

set(ENGINE_SIM_SOURCES
    ${ENGINE_SIM_ROOT}/src/camshaft.cpp
    ${ENGINE_SIM_ROOT}/src/combustion_chamber.cpp
    ${ENGINE_SIM_ROOT}/src/connecting_rod.cpp
    ${ENGINE_SIM_ROOT}/src/convolution_filter.cpp
    ${ENGINE_SIM_ROOT}/src/crankshaft.cpp
    ${ENGINE_SIM_ROOT}/src/cylinder_bank.cpp
    ${ENGINE_SIM_ROOT}/src/cylinder_head.cpp
    ${ENGINE_SIM_ROOT}/src/delay_filter.cpp
    ${ENGINE_SIM_ROOT}/src/derivative_filter.cpp
    ${ENGINE_SIM_ROOT}/src/direct_throttle_linkage.cpp
    ${ENGINE_SIM_ROOT}/src/engine.cpp
    ${ENGINE_SIM_ROOT}/src/exhaust_system.cpp
    ${ENGINE_SIM_ROOT}/src/filter.cpp
    ${ENGINE_SIM_ROOT}/src/fuel.cpp
    ${ENGINE_SIM_ROOT}/src/function.cpp
    ${ENGINE_SIM_ROOT}/src/gas_system.cpp
    ${ENGINE_SIM_ROOT}/src/gaussian_filter.cpp
    ${ENGINE_SIM_ROOT}/src/ignition_module.cpp
    ${ENGINE_SIM_ROOT}/src/impulse_response.cpp
    ${ENGINE_SIM_ROOT}/src/intake.cpp
    ${ENGINE_SIM_ROOT}/src/jitter_filter.cpp
    ${ENGINE_SIM_ROOT}/src/leveling_filter.cpp
    ${ENGINE_SIM_ROOT}/src/low_pass_filter.cpp
    ${ENGINE_SIM_ROOT}/src/part.cpp
    ${ENGINE_SIM_ROOT}/src/piston.cpp
    ${ENGINE_SIM_ROOT}/src/piston_engine_simulator.cpp
    ${ENGINE_SIM_ROOT}/src/simulator.cpp
    ${ENGINE_SIM_ROOT}/src/standard_valvetrain.cpp
    ${ENGINE_SIM_ROOT}/src/starter_motor.cpp
    ${ENGINE_SIM_ROOT}/src/synthesizer.cpp
    ${ENGINE_SIM_ROOT}/src/throttle.cpp
    ${ENGINE_SIM_ROOT}/src/utilities.cpp
    ${ENGINE_SIM_ROOT}/src/valvetrain.cpp
)

add_library(nc003b_solver STATIC ${SOLVER_SOURCES})
target_include_directories(nc003b_solver PUBLIC ${SOLVER_ROOT}/include)

add_library(nc003b_engine_sim STATIC ${ENGINE_SIM_SOURCES})
target_include_directories(nc003b_engine_sim PUBLIC
    ${ENGINE_SIM_ROOT}/include
    ${SOLVER_ROOT}/include
)
target_link_libraries(nc003b_engine_sim PUBLIC nc003b_solver)

add_library(nc003b_fixture STATIC
    ${CORE_ROOT}/Private/Fixtures/SubaruEJ25AtgVideo2Fixture.cpp
)
target_include_directories(nc003b_fixture PUBLIC
    ${CORE_ROOT}/Private/Fixtures
    ${ENGINE_SIM_ROOT}/include
    ${SOLVER_ROOT}/include
)
target_link_libraries(nc003b_fixture PUBLIC nc003b_engine_sim)

add_executable(nc003b_phase0_tests tests/phase0_standalone_tests.cpp)
target_link_libraries(nc003b_phase0_tests PRIVATE nc003b_fixture)

if(MSVC)
    target_compile_options(nc003b_solver PRIVATE /W4)
    target_compile_options(nc003b_engine_sim PRIVATE /W4)
    target_compile_options(nc003b_fixture PRIVATE /W4)
    target_compile_options(nc003b_phase0_tests PRIVATE /W4)
else()
    # Pinned solver headers use the MSVC spelling. Keep the compatibility
    # definition visible and confined to this standalone harness.
    target_compile_definitions(nc003b_solver PUBLIC __forceinline=inline)
    foreach(target nc003b_solver nc003b_engine_sim nc003b_fixture nc003b_phase0_tests)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
endif()

enable_testing()
add_test(NAME nc003b_phase0_tests COMMAND nc003b_phase0_tests)
