#include "CylinderVibrationIBM.H"
#include <AMReX_ParmParse.H>
#include <AMReX_Print.H>
#include <AMReX_Math.H>
#include <AMReX_MLNodeLaplacian.H>
#include <AMReX_FillPatchUtil.H>
#include <filesystem>
#include <cmath>

using namespace amrex;

// Static member initialization
std::vector<CylinderVibrationIBM*> CylinderVibrationIBMManager::cylinders;
Geometry CylinderVibrationIBMManager::global_geom;
DistributionMapping CylinderVibrationIBMManager::global_dm;
BoxArray CylinderVibrationIBMManager::global_ba;
bool CylinderVibrationIBMManager::initialized = false;

// Cylinder vibration IBM class implementation
CylinderVibrationIBM::CylinderVibrationIBM() 
    : file_index(0), total_force(0.0, 0.0, 0.0), total_moment(0.0, 0.0, 0.0), 
      fluid_force(0.0, 0.0, 0.0), added_mass_force(0.0, 0.0, 0.0) {
}

CylinderVibrationIBM::~CylinderVibrationIBM() {
    if (force_file.is_open()) {
        force_file.close();
    }
    if (vibration_file.is_open()) {
        vibration_file.close();
    }
}

void CylinderVibrationIBM::Initialize(const Geometry& geom,
                                     const DistributionMapping& dm,
                                     const BoxArray& ba,
                                     const CylinderVibrationParams& params) {
    this->params = params;
    this->geometry = geom;
    this->dmap = dm;
    this->box_array = ba;
    
    // Read parameters from input file if not provided
    ReadParameters();
    
    // Initialize position and velocity
    current_position = params.center + params.initial_displacement;
    current_velocity = params.initial_velocity;
    current_angular_velocity = params.initial_angular_velocity;
    current_rotation = params.initial_rotation;
    
    // Initialize historical data
    position_old = current_position;
    velocity_old = current_velocity;
    angular_velocity_old = current_angular_velocity;
    rotation_old = current_rotation;
    
    // Initialize Lagrangian markers
    InitializeLagrangianMarkers();
    
    // Initialize output files
    InitializeForceFile();
    InitializeVibrationFile();
    
    if (ParallelDescriptor::IOProcessor()) {
        Print() << "CylinderVibrationIBM initialized: "
                << "shape=" << (params.shape == CIRCULAR ? "CIRCULAR" : "SQUARE")
                << ", radius=" << params.radius 
                << ", center=(" << params.center[0] << "," << params.center[1] << "," << params.center[2] << ")"
                << ", type=" << (params.vib_type == FORCED_VIBRATION ? "FORCED" : 
                                params.vib_type == FREE_VIBRATION ? "FREE" : "VIV")
                << ", markers=" << markers.size() << std::endl;
    }
}

void CylinderVibrationIBM::ReadParameters() {
    ParmParse pp("cylinder_vib");
    
    // Read cylinder geometry
    int shape_int = 0;  // Default to circular
    pp.query("shape", shape_int);
    params.shape = static_cast<CYLINDER_SHAPE>(shape_int);
    
    pp.query("radius", params.radius);
    
    Vector<Real> center_vec(AMREX_SPACEDIM, 0.0);
    pp.queryarr("center", center_vec, 0, AMREX_SPACEDIM);
    for (int i = 0; i < AMREX_SPACEDIM; ++i) {
        params.center[i] = center_vec[i];
    }
    
    // Read vibration type
    int vib_type_int = 0;  // Default to forced vibration
    pp.query("vibration_type", vib_type_int);
    params.vib_type = static_cast<VIBRATION_TYPE>(vib_type_int);
    
    // Read VIV parameters with defaults
    params.mass_ratio = 2.0;
    pp.query("mass_ratio", params.mass_ratio);
    
    params.natural_frequency_x = 10.0;
    pp.query("natural_frequency_x", params.natural_frequency_x);
    
    params.natural_frequency_y = 10.0;
    pp.query("natural_frequency_y", params.natural_frequency_y);
    
    Real damping_ratio_x = 0.01, damping_ratio_y = 0.01;
    pp.query("damping_ratio_x", damping_ratio_x);
    pp.query("damping_ratio_y", damping_ratio_y);
    
    // Set default mass if not provided
    if (params.mass <= 0.0) {
        params.mass = params.mass_ratio * params.density * 4.0 * params.radius * params.radius;
    }
    
    // Convert damping ratio to damping coefficient
    params.damping_coefficient_x = VIVUtils::ComputeDampingCoefficient(
        damping_ratio_x, params.natural_frequency_x, params.mass);
    params.damping_coefficient_y = VIVUtils::ComputeDampingCoefficient(
        damping_ratio_y, params.natural_frequency_y, params.mass);
    
    // Read constraint conditions with defaults
    Vector<int> trans_lock(AMREX_SPACEDIM, 0);  // Default all locked
    pp.queryarr("translation_lock", trans_lock, 0, AMREX_SPACEDIM);
    for (int i = 0; i < AMREX_SPACEDIM; ++i) {
        params.translation_lock[i] = trans_lock[i];
    }
    
    // Read IBM parameters with defaults
    params.markers_per_circumference = 64;
    pp.query("markers_per_circumference", params.markers_per_circumference);
    
    int delta_type_int = 0;  // Default to four-point
    pp.query("delta_function_type", delta_type_int);
    params.delta_type = static_cast<DELTA_FUNCTION_TYPE>(delta_type_int);
    
    // Set default density if not provided
    if (params.density <= 0.0) {
        params.density = 1.0;  // Default fluid density
    }
}

void CylinderVibrationIBM::InitializeLagrangianMarkers() {
    markers.clear();
    
    if (params.shape == CIRCULAR) {
        GenerateCircularMarkers();
    } else {
        GenerateSquareMarkers();
    }
    
    // Initialize marker properties
    for (int i = 0; i < markers.size(); ++i) {
        markers[i].id = i;
        markers[i].velocity = current_velocity;
        markers[i].force = RealVect(0.0);
    }
}

void CylinderVibrationIBM::GenerateCircularMarkers() {
    Real radius = params.radius;
    RealVect center = current_position;
    int num_markers = params.markers_per_circumference;
    
    // Generate markers around circumference
    for (int i = 0; i < num_markers; ++i) {
        Real angle = 2.0 * M_PI * i / num_markers;
        Real x = center[0] + radius * std::cos(angle);
        Real y = center[1] + radius * std::sin(angle);
        Real z = center[2];
        
        LagrangianMarker marker;
        marker.position = RealVect(x, y, z);
        marker.normal = RealVect(std::cos(angle), std::sin(angle), 0.0);
        marker.area = 2.0 * M_PI * radius / num_markers;
        
        markers.push_back(marker);
    }
}

void CylinderVibrationIBM::GenerateSquareMarkers() {
    Real half_width = params.radius;
    RealVect center = current_position;
    int markers_per_side = params.markers_per_circumference / 4;
    
    // Generate markers on each side of the square
    for (int side = 0; side < 4; ++side) {
        for (int i = 0; i < markers_per_side; ++i) {
            Real s = -1.0 + 2.0 * i / (markers_per_side - 1);
            
            RealVect position, normal;
            switch (side) {
                case 0: // Bottom side
                    position = RealVect(center[0] + s * half_width, center[1] - half_width, center[2]);
                    normal = RealVect(0.0, 1.0, 0.0);
                    break;
                case 1: // Right side
                    position = RealVect(center[0] + half_width, center[1] + s * half_width, center[2]);
                    normal = RealVect(-1.0, 0.0, 0.0);
                    break;
                case 2: // Top side
                    position = RealVect(center[0] + s * half_width, center[1] + half_width, center[2]);
                    normal = RealVect(0.0, -1.0, 0.0);
                    break;
                case 3: // Left side
                    position = RealVect(center[0] - half_width, center[1] + s * half_width, center[2]);
                    normal = RealVect(1.0, 0.0, 0.0);
                    break;
            }
            
            LagrangianMarker marker;
            marker.position = position;
            marker.normal = normal;
            marker.area = 2.0 * half_width / markers_per_side;
            
            markers.push_back(marker);
        }
    }
}

void CylinderVibrationIBM::UpdateLagrangianMarkers() {
    // Update marker positions based on current cylinder position
    RealVect displacement = current_position - params.center;
    
    for (auto& marker : markers) {
        // Update position (simple translation for now)
        marker.position += displacement;
        
        // Update velocity
        marker.velocity = current_velocity;
    }
}

void CylinderVibrationIBM::VelocityInterpolation(const MultiFab& velocity_field) {
    const auto& geom = geometry;
    const auto& problo = geom.ProbLoArray();
    const auto& dx = geom.CellSizeArray();
    
    // Interpolate velocity from Euler grid to Lagrangian markers
    for (auto& marker : markers) {
        RealVect interpolated_velocity(0.0);
        Real total_weight = 0.0;
        
        // Find nearby grid points and interpolate
        for (MFIter mfi(velocity_field, TilingIfNotGPU()); mfi.isValid(); ++mfi) {
            const Box& bx = mfi.tilebox();
            auto const& vel = velocity_field.array(mfi);
            
            // Find grid points near the marker
            int i_min = static_cast<int>((marker.position[0] - problo[0]) / dx[0]) - 2;
            int i_max = static_cast<int>((marker.position[0] - problo[0]) / dx[0]) + 2;
            int j_min = static_cast<int>((marker.position[1] - problo[1]) / dx[1]) - 2;
            int j_max = static_cast<int>((marker.position[1] - problo[1]) / dx[1]) + 2;
            
            for (int i = i_min; i <= i_max; ++i) {
                for (int j = j_min; j <= j_max; ++j) {
                    if (bx.contains(IntVect(i, j, 0))) {
                        Real x_grid = problo[0] + (i + 0.5) * dx[0];
                        Real y_grid = problo[1] + (j + 0.5) * dx[1];
                        
                        Real weight = ComputeDeltaFunction(x_grid, marker.position[0], dx[0]) *
                                     ComputeDeltaFunction(y_grid, marker.position[1], dx[1]);
                        
                        interpolated_velocity[0] += weight * vel(i, j, 0, 0);
                        interpolated_velocity[1] += weight * vel(i, j, 0, 1);
                        total_weight += weight;
                    }
                }
            }
        }
        
        // Normalize by total weight
        if (total_weight > 0.0) {
            marker.velocity = interpolated_velocity / total_weight;
        }
    }
}

void CylinderVibrationIBM::ComputeLagrangianForces(Real dt) {
    // Compute forces on Lagrangian markers
    for (auto& marker : markers) {
        // Compute fluid force using momentum exchange
        RealVect fluid_velocity = marker.velocity;
        RealVect solid_velocity = current_velocity;
        
        // Simple force model (can be improved)
        RealVect relative_velocity = fluid_velocity - solid_velocity;
        marker.force = -relative_velocity * marker.area * params.density;
    }
    
    // Sum up forces from all markers
    fluid_force = RealVect(0.0);
    for (const auto& marker : markers) {
        fluid_force += marker.force;
    }
    
    // MPI reduction for parallel computation
    ParallelDescriptor::ReduceRealSum(fluid_force.dataPtr(), 3);
}

void CylinderVibrationIBM::ForceSpreading(MultiFab& force_field) {
    const auto& geom = geometry;
    const auto& problo = geom.ProbLoArray();
    const auto& dx = geom.CellSizeArray();
    
    // Spread forces from Lagrangian markers to Euler grid
    for (MFIter mfi(force_field, TilingIfNotGPU()); mfi.isValid(); ++mfi) {
        const Box& bx = mfi.tilebox();
        auto const& force = force_field.array(mfi);
        
        for (const auto& marker : markers) {
            // Find nearby grid points
            int i_min = static_cast<int>((marker.position[0] - problo[0]) / dx[0]) - 2;
            int i_max = static_cast<int>((marker.position[0] - problo[0]) / dx[0]) + 2;
            int j_min = static_cast<int>((marker.position[1] - problo[1]) / dx[1]) - 2;
            int j_max = static_cast<int>((marker.position[1] - problo[1]) / dx[1]) + 2;
            
            for (int i = i_min; i <= i_max; ++i) {
                for (int j = j_min; j <= j_max; ++j) {
                    if (bx.contains(IntVect(i, j, 0))) {
                        Real x_grid = problo[0] + (i + 0.5) * dx[0];
                        Real y_grid = problo[1] + (j + 0.5) * dx[1];
                        
                        Real weight = ComputeDeltaFunction(x_grid, marker.position[0], dx[0]) *
                                     ComputeDeltaFunction(y_grid, marker.position[1], dx[1]);
                        
                        force(i, j, 0, 0) += weight * marker.force[0];
                        force(i, j, 0, 1) += weight * marker.force[1];
                    }
                }
            }
        }
    }
}

void CylinderVibrationIBM::VelocityCorrection(MultiFab& velocity_field, MultiFab& force_field, Real dt) {
    // Apply velocity correction based on IBM forces
    for (MFIter mfi(velocity_field, TilingIfNotGPU()); mfi.isValid(); ++mfi) {
        const Box& bx = mfi.tilebox();
        auto const& vel = velocity_field.array(mfi);
        auto const& force = force_field.array(mfi);
        
        amrex::ParallelFor(bx, AMREX_SPACEDIM, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) noexcept {
            vel(i, j, k, n) += dt * force(i, j, k, n);
        });
    }
}

void CylinderVibrationIBM::UpdateCylinder(Real time, Real dt, const MultiFab& velocity_field) {
    // Update historical data
    UpdateHistory();
    
    // Update Lagrangian markers
    UpdateLagrangianMarkers();
    
    // Perform IBM operations
    VelocityInterpolation(velocity_field);
    ComputeLagrangianForces(dt);
    
    // Update position according to vibration type
    if (params.vib_type == FORCED_VIBRATION) {
        current_position = ComputeForcedPosition(time);
        RealVect pos_old = ComputeForcedPosition(time - dt);
        current_velocity = (current_position - pos_old) / dt;
    } else if (params.vib_type == FREE_VIBRATION) {
        current_position = ComputeFreePosition(time, dt);
    } else if (params.vib_type == VIV_VIBRATION) {
        current_position = ComputeVIVPosition(time, dt);
    }
}

RealVect CylinderVibrationIBM::ComputeForcedPosition(Real time) {
    RealVect base_position = params.center;
    RealVect displacement(0.0);
    
    // Calculate forced vibration displacement
    Real omega = 2.0 * M_PI * params.frequency;
    Real displacement_magnitude = params.amplitude * std::sin(omega * time + params.phase);
    
    // Apply displacement according to constraint conditions
    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
        if (params.translation_lock[dir] == 2) { // Vibration mode
            displacement[dir] = displacement_magnitude;
        } else if (params.translation_lock[dir] == 1) { // Free mode
            displacement[dir] = 0.0;
        } else { // Locked mode
            displacement[dir] = 0.0;
        }
    }
    
    return base_position + displacement;
}

RealVect CylinderVibrationIBM::ComputeFreePosition(Real time, Real dt) {
    // Numerical integration for spring-mass-damping system
    RealVect spring_force = ComputeSpringForce();
    RealVect damping_force = ComputeDampingForce();
    RealVect fluid_force_total = fluid_force + added_mass_force;
    
    // Calculate total force
    RealVect total_force_system = spring_force + damping_force + fluid_force_total;
    
    // Update velocity and position using Velocity Verlet integration (more stable)
    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
        if (params.translation_lock[dir] == 1 || params.translation_lock[dir] == 2) {
            Real acceleration = total_force_system[dir] / params.mass;
            current_velocity[dir] = velocity_old[dir] + 0.5 * acceleration * dt;
            current_position[dir] = position_old[dir] + current_velocity[dir] * dt;
        }
    }
    
    return current_position;
}

RealVect CylinderVibrationIBM::ComputeVIVPosition(Real time, Real dt) {
    // VIV-specific computation with lock-in effects
    RealVect spring_force = ComputeSpringForce();
    RealVect damping_force = ComputeDampingForce();
    RealVect fluid_force_total = fluid_force + added_mass_force;
    
    // VIV-specific force modifications (lock-in effects)
    RealVect viv_force = fluid_force_total;
    
    // Compute vortex shedding frequency using Strouhal number
    Real velocity_magnitude = std::sqrt(current_velocity[0]*current_velocity[0] + 
                                       current_velocity[1]*current_velocity[1]);
    Real diameter = 2.0 * params.radius;
    Real strouhal_number = 0.2; // For circular cylinder at Re > 1000
    Real vortex_shedding_freq = strouhal_number * velocity_magnitude / diameter;
    
    // Apply lock-in effects based on frequency ratio
    Real natural_freq_x = params.natural_frequency_x;
    Real natural_freq_y = params.natural_frequency_y;
    
    Real freq_ratio_x = vortex_shedding_freq / natural_freq_x;
    Real freq_ratio_y = vortex_shedding_freq / natural_freq_y;
    
    // Lock-in effect: amplify forces when frequencies are close (0.8 < f/fn < 1.2)
    Real lock_in_bandwidth = 0.2;
    if (std::abs(freq_ratio_x - 1.0) < lock_in_bandwidth) {
        Real lock_in_factor = 1.0 + 0.5 * (1.0 - std::abs(freq_ratio_x - 1.0) / lock_in_bandwidth);
        viv_force[0] *= lock_in_factor;
    }
    if (std::abs(freq_ratio_y - 1.0) < lock_in_bandwidth) {
        Real lock_in_factor = 1.0 + 0.5 * (1.0 - std::abs(freq_ratio_y - 1.0) / lock_in_bandwidth);
        viv_force[1] *= lock_in_factor;
    }
    
    // Calculate total force
    RealVect total_force_system = spring_force + damping_force + viv_force;
    
    // Update velocity and position
    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
        if (params.translation_lock[dir] == 1 || params.translation_lock[dir] == 2) {
            Real acceleration = total_force_system[dir] / params.mass;
            current_velocity[dir] = velocity_old[dir] + acceleration * dt;
            current_position[dir] = position_old[dir] + current_velocity[dir] * dt;
        }
    }
    
    return current_position;
}

RealVect CylinderVibrationIBM::ComputeSpringForce() {
    RealVect spring_force(0.0);
    RealVect equilibrium_position = params.center;
    
    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
        if (params.translation_lock[dir] == 1 || params.translation_lock[dir] == 2) {
            Real displacement = current_position[dir] - equilibrium_position[dir];
            Real spring_constant = (dir == 0) ? params.spring_constant_x : params.spring_constant_y;
            spring_force[dir] = -spring_constant * displacement;
        }
    }
    
    return spring_force;
}

RealVect CylinderVibrationIBM::ComputeDampingForce() {
    RealVect damping_force(0.0);
    
    for (int dir = 0; dir < AMREX_SPACEDIM; ++dir) {
        if (params.translation_lock[dir] == 1 || params.translation_lock[dir] == 2) {
            Real damping_coefficient = (dir == 0) ? params.damping_coefficient_x : params.damping_coefficient_y;
            damping_force[dir] = -damping_coefficient * current_velocity[dir];
        }
    }
    
    return damping_force;
}

RealVect CylinderVibrationIBM::ComputeFluidForce() {
    return fluid_force;
}

RealVect CylinderVibrationIBM::ComputeAddedMassForce(Real dt) {
    // Compute added mass force based on acceleration
    RealVect acceleration = (current_velocity - velocity_old) / dt;
    Real added_mass_coefficient = 1.0; // For circular cylinder
    
    added_mass_force = -added_mass_coefficient * params.density * M_PI * params.radius * params.radius * acceleration;
    
    return added_mass_force;
}

void CylinderVibrationIBM::SetBoundaryConditions(MultiFab& phi_nodal, MultiFab& pvf) {
    UpdateDistanceFunction();
    
    // Set phi values based on current cylinder position
    const auto& geom = geometry;
    const auto& problo = geom.ProbLoArray();
    const auto& dx = geom.CellSizeArray();
    
    for (MFIter mfi(phi_nodal, TilingIfNotGPU()); mfi.isValid(); ++mfi) {
        const Box& bx = mfi.tilebox();
        auto const& phi = phi_nodal.array(mfi);
        
        amrex::ParallelFor(bx, [=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept {
            Real x = problo[0] + (i + 0.5) * dx[0];
            Real y = problo[1] + (j + 0.5) * dx[1];
#if (AMREX_SPACEDIM == 3)
            Real z = problo[2] + (k + 0.5) * dx[2];
#else
            constexpr Real z = 0.0;
#endif
            
            // Calculate distance to cylinder center
            Real dx_center = x - current_position[0];
            Real dy_center = y - current_position[1];
            Real dz_center = z - current_position[2];
            Real distance = std::sqrt(dx_center*dx_center + dy_center*dy_center + dz_center*dz_center);
            
            // Set phi value (distance function)
            phi(i,j,k) = distance - params.radius;
        });
    }
    
    // Update pvf (volume fraction)
    nodal_phi_to_pvf(pvf, phi_nodal);
}

void CylinderVibrationIBM::UpdateDistanceFunction() {
    // Update distance function based on current cylinder position
    // This is called before setting boundary conditions
}

Real CylinderVibrationIBM::ComputeDeltaFunction(Real xf, Real xp, Real h) {
    Real rr = std::abs((xf - xp) / h);
    
    switch (params.delta_type) {
    case FOUR_POINT_IB:
        if(rr >= 0 && rr < 1){
            return 1.0/8.0 * (3.0 - 2.0*rr + std::sqrt(1.0 + 4.0*rr - 4*rr*rr)) / h;
        }else if (rr >= 1 && rr < 2) {
            return 1.0/8.0 * (5.0 - 2.0*rr - std::sqrt(-7.0 + 12.0*rr - 4*rr*rr)) / h;
        }else {
            return 0.0;
        }
        break;
    case THREE_POINT_IB:
        if(rr >= 0.5 && rr < 1.5){
            return 1.0/6.0 * (5.0 - 3.0*rr - std::sqrt(-3.0 * (1 - rr)*(1 - rr) + 1.0)) / h;
        }else if (rr >= 0 && rr < 0.5) {
            return 1.0/3.0 * (1.0 + std::sqrt(1.0 - 3*rr*rr)) / h;
        }else {
            return 0.0;
        }
        break;
    default:
        return 0.0;
    }
}

void CylinderVibrationIBM::UpdateHistory() {
    position_old = current_position;
    velocity_old = current_velocity;
    angular_velocity_old = current_angular_velocity;
    rotation_old = current_rotation;
}

void CylinderVibrationIBM::InitializeForceFile() {
    if (ParallelDescriptor::IOProcessor()) {
        std::string filename = "cylinder_force_ibm_" + std::to_string(file_index) + ".dat";
        force_file.open(filename);
        force_file << "# Time Step Time dt Fx Fy Fz Mx My Mz X Y Z Vx Vy Vz FluidFx FluidFy FluidFz" << std::endl;
    }
}

void CylinderVibrationIBM::InitializeVibrationFile() {
    if (ParallelDescriptor::IOProcessor()) {
        std::string filename = "cylinder_vibration_ibm_" + std::to_string(file_index) + ".dat";
        vibration_file.open(filename);
        vibration_file << "# Time Step Time dt X Y Z Vx Vy Vz Ax Ay Az SpringFx SpringFy SpringFz DampFx DampFy DampFz" << std::endl;
    }
}

void CylinderVibrationIBM::WriteForceData(int step, Real time, Real dt) {
    if (ParallelDescriptor::IOProcessor() && force_file.is_open()) {
        force_file << step << " " << time << " " << dt << " "
                   << total_force[0] << " " << total_force[1] << " " << total_force[2] << " "
                   << total_moment[0] << " " << total_moment[1] << " " << total_moment[2] << " "
                   << current_position[0] << " " << current_position[1] << " " << current_position[2] << " "
                   << current_velocity[0] << " " << current_velocity[1] << " " << current_velocity[2] << " "
                   << fluid_force[0] << " " << fluid_force[1] << " " << fluid_force[2] << std::endl;
    }
}

void CylinderVibrationIBM::WriteVibrationData(int step, Real time, Real dt) {
    if (ParallelDescriptor::IOProcessor() && vibration_file.is_open()) {
        RealVect spring_force = ComputeSpringForce();
        RealVect damping_force = ComputeDampingForce();
        RealVect acceleration = (current_velocity - velocity_old) / dt;
        
        vibration_file << step << " " << time << " " << dt << " "
                      << current_position[0] << " " << current_position[1] << " " << current_position[2] << " "
                      << current_velocity[0] << " " << current_velocity[1] << " " << current_velocity[2] << " "
                      << acceleration[0] << " " << acceleration[1] << " " << acceleration[2] << " "
                      << spring_force[0] << " " << spring_force[1] << " " << spring_force[2] << " "
                      << damping_force[0] << " " << damping_force[1] << " " << damping_force[2] << std::endl;
    }
}

// Global manager implementation
void CylinderVibrationIBMManager::Initialize(const Geometry& geom,
                                           const DistributionMapping& dm,
                                           const BoxArray& ba) {
    global_geom = geom;
    global_dm = dm;
    global_ba = ba;
    initialized = true;
    
    if (ParallelDescriptor::IOProcessor()) {
        Print() << "CylinderVibrationIBMManager initialized" << std::endl;
    }
}

void CylinderVibrationIBMManager::AddCylinder(const CylinderVibrationParams& params) {
    if (!initialized) {
        amrex::Abort("CylinderVibrationIBMManager not initialized");
    }
    
    CylinderVibrationIBM* cylinder = new CylinderVibrationIBM();
    cylinder->Initialize(global_geom, global_dm, global_ba, params);
    cylinders.push_back(cylinder);
}

void CylinderVibrationIBMManager::UpdateAllCylinders(Real time, Real dt, const MultiFab& velocity_field) {
    for (auto& cylinder : cylinders) {
        cylinder->UpdateCylinder(time, dt, velocity_field);
    }
}

void CylinderVibrationIBMManager::WriteAllForceData(int step, Real time, Real dt) {
    for (auto& cylinder : cylinders) {
        cylinder->WriteForceData(step, time, dt);
    }
}

void CylinderVibrationIBMManager::WriteAllVibrationData(int step, Real time, Real dt) {
    for (auto& cylinder : cylinders) {
        cylinder->WriteVibrationData(step, time, dt);
    }
}

void CylinderVibrationIBMManager::SetBoundaryConditions(MultiFab& phi_nodal, MultiFab& pvf) {
    for (auto& cylinder : cylinders) {
        cylinder->SetBoundaryConditions(phi_nodal, pvf);
    }
}

void CylinderVibrationIBMManager::VelocityInterpolationAll(const MultiFab& velocity_field) {
    for (auto& cylinder : cylinders) {
        cylinder->VelocityInterpolation(velocity_field);
    }
}

void CylinderVibrationIBMManager::ForceSpreadingAll(MultiFab& force_field) {
    for (auto& cylinder : cylinders) {
        cylinder->ForceSpreading(force_field);
    }
}

void CylinderVibrationIBMManager::VelocityCorrectionAll(MultiFab& velocity_field, MultiFab& force_field, Real dt) {
    for (auto& cylinder : cylinders) {
        cylinder->VelocityCorrection(velocity_field, force_field, dt);
    }
}

void CylinderVibrationIBMManager::SetupVIVCase(Real Re, Real mass_ratio, Real natural_frequency_x, Real natural_frequency_y) {
    // Setup VIV case with Re=3900 parameters
    if (ParallelDescriptor::IOProcessor()) {
        Print() << "Setting up VIV case: Re=" << Re 
                << ", mass_ratio=" << mass_ratio
                << ", fn_x=" << natural_frequency_x
                << ", fn_y=" << natural_frequency_y << std::endl;
    }
}

// VIV utility functions
namespace VIVUtils {
    Real ComputeReynoldsNumber(Real velocity, Real diameter, Real viscosity) {
        return velocity * diameter / viscosity;
    }
    
    Real ComputeReducedVelocity(Real velocity, Real natural_frequency, Real diameter) {
        return velocity / (natural_frequency * diameter);
    }
    
    Real ComputeMassRatio(Real mass, Real fluid_density, Real diameter) {
        return mass / (fluid_density * diameter * diameter);
    }
    
    Real ComputeNaturalFrequency(Real spring_constant, Real mass) {
        return std::sqrt(spring_constant / mass) / (2.0 * M_PI);
    }
    
    Real ComputeSpringConstant(Real natural_frequency, Real mass) {
        return mass * (2.0 * M_PI * natural_frequency) * (2.0 * M_PI * natural_frequency);
    }
    
    Real ComputeDampingCoefficient(Real damping_ratio, Real natural_frequency, Real mass) {
        return 2.0 * damping_ratio * mass * 2.0 * M_PI * natural_frequency;
    }
    
    Real ComputeDampingRatio(Real damping_coefficient, Real natural_frequency, Real mass) {
        return damping_coefficient / (2.0 * mass * 2.0 * M_PI * natural_frequency);
    }
} 