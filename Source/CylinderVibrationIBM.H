#ifndef CYLINDERVIBRATIONIBM_H
#define CYLINDERVIBRATIONIBM_H

#include <AMReX_Particles.H>
#include <AMReX_MultiFabUtil.H>
#include <AMReX_RealVect.H>
#include <AMReX_MLNodeLaplacian.H>
#include <AMReX_FillPatchUtil.H>
#include <fstream>
#include "DiffusedIB.H"

// Forward declaration
enum DELTA_FUNCTION_TYPE;

using namespace amrex;

// Vibration type enumeration
enum VIBRATION_TYPE {
    FORCED_VIBRATION = 0,    // Forced vibration (prescribed motion)
    FREE_VIBRATION = 1,      // Free vibration (spring-mass-damping)
    VIV_VIBRATION = 2        // Vortex-Induced Vibration (VIV)
};

// Cylinder shape enumeration
enum CYLINDER_SHAPE {
    CIRCULAR = 0,            // Circular cylinder
    SQUARE = 1               // Square cylinder
};

    // Cylinder vibration parameters structure
    struct CylinderVibrationParams {
        // Basic geometry
        CYLINDER_SHAPE shape;    // Cylinder shape (circular or square)
        Real radius;             // Radius (for circular) or half-width (for square)
        RealVect center;         // Cylinder center position
        Real density;            // Cylinder density
        Real mass;               // Cylinder mass
        Real moment_of_inertia;  // Moment of inertia
        
        // Constructor with defaults
        CylinderVibrationParams() 
            : shape(CIRCULAR), radius(0.01), center(0.0, 0.0, 0.0), density(1.0), 
              mass(0.0), moment_of_inertia(0.0), vib_type(FORCED_VIBRATION),
              amplitude(0.0), frequency(1.0), phase(0.0),
              spring_constant_x(0.0), spring_constant_y(0.0),
              damping_coefficient_x(0.0), damping_coefficient_y(0.0),
              natural_frequency_x(10.0), natural_frequency_y(10.0),
              mass_ratio(2.0), reduced_velocity(0.0),
              translation_lock(0, 0, 0), rotation_lock(0, 0, 0),
              initial_velocity(0.0, 0.0, 0.0), initial_angular_velocity(0.0, 0.0, 0.0),
              initial_displacement(0.0, 0.0, 0.0), initial_rotation(0.0, 0.0, 0.0),
              marker_spacing(0.0), markers_per_circumference(64),
              delta_type(FOUR_POINT_IB) {}
    
    // Vibration parameters
    VIBRATION_TYPE vib_type;
    Real amplitude;          // Vibration amplitude
    Real frequency;          // Vibration frequency
    Real phase;              // Initial phase
    
    // Spring-damping system parameters (for free vibration and VIV)
    Real spring_constant_x;  // Spring constant in x direction
    Real spring_constant_y;  // Spring constant in y direction
    Real damping_coefficient_x; // Damping coefficient in x direction
    Real damping_coefficient_y; // Damping coefficient in y direction
    
    // Natural frequency parameters (for VIV)
    Real natural_frequency_x; // Natural frequency in x direction
    Real natural_frequency_y; // Natural frequency in y direction
    Real mass_ratio;         // Mass ratio (m/ρD²)
    Real reduced_velocity;   // Reduced velocity (U/fnD)
    
    // Constraint conditions
    IntVect translation_lock; // Translation DOF lock (0=locked, 1=free, 2=vibration)
    IntVect rotation_lock;    // Rotation DOF lock
    
    // Initial conditions
    RealVect initial_velocity;
    RealVect initial_angular_velocity;
    RealVect initial_displacement;
    RealVect initial_rotation;
    
    // IBM parameters
    Real marker_spacing;     // Lagrangian marker spacing
    int markers_per_circumference; // Number of markers around circumference
    DELTA_FUNCTION_TYPE delta_type; // Delta function type
};

// Lagrangian marker structure for IBM
struct LagrangianMarker {
    RealVect position;       // Current position
    RealVect velocity;       // Current velocity
    RealVect force;          // Force on marker
    RealVect normal;         // Surface normal
    Real area;               // Marker area
    int id;                  // Marker ID
};

// Cylinder vibration class with full IBM implementation
class CylinderVibrationIBM {
public:
    CylinderVibrationIBM();
    ~CylinderVibrationIBM();
    
    // Initialize cylinder vibration with IBM
    void Initialize(const Geometry& geom,
                   const DistributionMapping& dm,
                   const BoxArray& ba,
                   const CylinderVibrationParams& params);
    
    // IBM core functions
    void InitializeLagrangianMarkers();
    void UpdateLagrangianMarkers();
    void VelocityInterpolation(const MultiFab& velocity_field);
    void ComputeLagrangianForces(Real dt);
    void ForceSpreading(MultiFab& force_field);
    void VelocityCorrection(MultiFab& velocity_field, MultiFab& force_field, Real dt);
    
    // Vibration computation
    void UpdateCylinder(Real time, Real dt, const MultiFab& velocity_field);
    RealVect ComputeForcedPosition(Real time);
    RealVect ComputeFreePosition(Real time, Real dt);
    RealVect ComputeVIVPosition(Real time, Real dt);
    
    // Force computation
    RealVect ComputeSpringForce();
    RealVect ComputeDampingForce();
    RealVect ComputeFluidForce();
    RealVect ComputeAddedMassForce(Real dt);
    
    // Boundary conditions
    void SetBoundaryConditions(MultiFab& phi_nodal, MultiFab& pvf);
    void UpdateDistanceFunction();
    
    // Data output
    void WriteForceData(int step, Real time, Real dt);
    void WriteVibrationData(int step, Real time, Real dt);
    
    // Getters
    RealVect GetCurrentPosition() const { return current_position; }
    RealVect GetCurrentVelocity() const { return current_velocity; }
    RealVect GetCurrentForce() const { return total_force; }
    Real GetRadius() const { return params.radius; }
    std::vector<LagrangianMarker>& GetMarkers() { return markers; }
    
private:
    CylinderVibrationParams params;
    RealVect current_position;
    RealVect current_velocity;
    RealVect current_angular_velocity;
    RealVect current_rotation;
    
    // Historical data
    RealVect position_old;
    RealVect velocity_old;
    RealVect angular_velocity_old;
    RealVect rotation_old;
    
    // Forces and moments
    RealVect total_force;
    RealVect total_moment;
    RealVect fluid_force;
    RealVect added_mass_force;
    
    // Lagrangian markers for IBM
    std::vector<LagrangianMarker> markers;
    
    // Geometry information
    Geometry geometry;
    DistributionMapping dmap;
    BoxArray box_array;
    
    // File output
    std::ofstream force_file;
    std::ofstream vibration_file;
    uint32_t file_index;
    
    // Helper functions
    void InitializeForceFile();
    void InitializeVibrationFile();
    void UpdateHistory();
    void GenerateCircularMarkers();
    void GenerateSquareMarkers();
    Real ComputeDeltaFunction(Real xf, Real xp, Real h);
    RealVect ComputeSurfaceNormal(const RealVect& point);
    void SynchronizeMarkers();
    void ReadParameters();
};

// Global cylinder vibration IBM manager
class CylinderVibrationIBMManager {
public:
    static void Initialize(const Geometry& geom,
                          const DistributionMapping& dm,
                          const BoxArray& ba);
    
    static void AddCylinder(const CylinderVibrationParams& params);
    
    static void UpdateAllCylinders(Real time, Real dt, const MultiFab& velocity_field);
    
    static void WriteAllForceData(int step, Real time, Real dt);
    
    static void WriteAllVibrationData(int step, Real time, Real dt);
    
    static void SetBoundaryConditions(MultiFab& phi_nodal, MultiFab& pvf);
    
    static void VelocityInterpolationAll(const MultiFab& velocity_field);
    
    static void ForceSpreadingAll(MultiFab& force_field);
    
    static void VelocityCorrectionAll(MultiFab& velocity_field, MultiFab& force_field, Real dt);
    
    static std::vector<CylinderVibrationIBM*>& GetCylinders() { return cylinders; }
    
    // VIV specific functions
    static void SetupVIVCase(Real Re, Real mass_ratio, Real natural_frequency_x, Real natural_frequency_y);
    
private:
    static std::vector<CylinderVibrationIBM*> cylinders;
    static Geometry global_geom;
    static DistributionMapping global_dm;
    static BoxArray global_ba;
    static bool initialized;
};

// Utility functions for VIV analysis
namespace VIVUtils {
    // Compute Reynolds number
    Real ComputeReynoldsNumber(Real velocity, Real diameter, Real viscosity);
    
    // Compute reduced velocity
    Real ComputeReducedVelocity(Real velocity, Real natural_frequency, Real diameter);
    
    // Compute mass ratio
    Real ComputeMassRatio(Real mass, Real fluid_density, Real diameter);
    
    // Compute natural frequency from spring constant and mass
    Real ComputeNaturalFrequency(Real spring_constant, Real mass);
    
    // Compute spring constant from natural frequency and mass
    Real ComputeSpringConstant(Real natural_frequency, Real mass);
    
    // Compute damping coefficient from damping ratio
    Real ComputeDampingCoefficient(Real damping_ratio, Real natural_frequency, Real mass);
    
    // Compute damping ratio from damping coefficient
    Real ComputeDampingRatio(Real damping_coefficient, Real natural_frequency, Real mass);
}

#endif // CYLINDERVIBRATIONIBM_H 