#ifndef DYNEARTHSOL3D_PARAMETERS_HPP
#define DYNEARTHSOL3D_PARAMETERS_HPP

#include <string>
#include <utility>
#include <vector>

#include "constants.hpp"
#include "array2d.hpp"

typedef std::pair<double,double> double_pair;

typedef std::vector<double> double_vec;
typedef std::vector<int> int_vec;
typedef std::vector<int_vec> int_vec2D;
typedef std::vector<uint> uint_vec;

typedef Array2D<double,NDIMS> array_t;
typedef Array2D<double,NSTR> tensor_t;
typedef Array2D<double,NODES_PER_ELEM> shapefn;
typedef Array2D<double,1> regattr_t;

typedef Array2D<int,NODES_PER_ELEM> conn_t;
typedef Array2D<int,NDIMS> segment_t;
typedef Array2D<int,1> segflag_t;

//
// Structures for input parameters
//
struct Sim {
    double max_time_in_yr;
    double output_time_interval_in_yr;
    int max_steps;
    int output_step_interval;
    int output_averaged_fields;
    int checkpoint_frame_interval;
    int restarting_from_frame;
    bool is_restarting;
    bool has_output_during_remeshing;
    bool has_marker_output;

    std::string modelname;
    std::string restarting_from_modelname;
};

struct Mesh {
    int meshing_option;
    int meshing_verbosity;
    int tetgen_optlevel;
    int quality_check_step_interval;

    double xlength, ylength, zlength;
    double resolution;
    double smallest_size;
    double largest_size;
    // for 2D only
    double min_angle;
    // for 3D only
    double min_tet_angle, max_ratio;
    double min_quality;

    double_pair refined_zonex, refined_zoney, refined_zonez;
    std::string poly_filename;

    int remeshing_option;
};

struct Control {
    double gravity;
    double characteristic_speed;
    double inertial_scaling;
    double dt_fraction;
    double damping_factor;
    int ref_pressure_option;

    int surface_process_option;
    double surface_diffusivity;

    bool is_quasi_static;
    bool has_thermal_diffusion;
};

struct BC {
    double surface_temperature;
    double mantle_temperature;

    double wrinkler_delta_rho;
    bool has_wrinkler_foundation;

    bool has_water_loading;

    int vbc_x0;
    int vbc_x1;
    int vbc_y0;
    int vbc_y1;
    int vbc_z0;
    int vbc_z1;

    double vbc_val_x0;
    double vbc_val_x1;
    double vbc_val_y0;
    double vbc_val_y1;
    double vbc_val_z0;
    double vbc_val_z1;
};

struct IC {
    int mattype_option;
    int weakzone_option;
    double weakzone_plstrain;
    double weakzone_azimuth;
    double weakzone_inclination;
    double weakzone_halfwidth;
    double weakzone_y_min;
    double weakzone_y_max;
    double weakzone_depth_min;
    double weakzone_depth_max;
    double weakzone_xcenter;
    double weakzone_ycenter;
    double weakzone_zcenter;
    double weakzone_xsemi_axis;
    double weakzone_ysemi_axis;
    double weakzone_zsemi_axis;

    double oceanic_plate_age_in_yr;
};

struct Mat {
    int rheol_type;
    int phase_change_option;
    int nmat;
    double visc_min;
    double visc_max;
    double tension_max;
    double therm_diff_max;

    double_vec rho0;
    double_vec alpha;

    double_vec bulk_modulus;
    double_vec shear_modulus;

    double_vec visc_exponent;
    double_vec visc_coefficient;
    double_vec visc_activation_energy;

    double_vec heat_capacity;
    double_vec therm_cond;

    // plastic parameters
    double_vec pls0, pls1;
    double_vec cohesion0, cohesion1;
    double_vec friction_angle0, friction_angle1;
    double_vec dilation_angle0, dilation_angle1;
};

struct Markers {
    int init_marker_option;
    int markers_per_element;
    double init_marker_spacing;
};

struct Param {
    Sim sim;
    Mesh mesh;
    Control control;
    BC bc;
    IC ic;
    Mat mat;
    Markers markers;
};


//
// Structures for model variables
//
class MatProps;
class MarkerSet;
struct Variables {
    double time;
    double dt;
    int steps;

    int nnode;
    int nelem;
    int nseg;

    double max_vbc_val;
    double compensation_pressure;

    // These 5 arrays are allocated by external library
    array_t *coord;
    conn_t *connectivity;
    segment_t *segment;
    segflag_t *segflag;
    regattr_t *regattr;

    uint_vec *bcflag;
    int_vec bnodes[6];
    std::vector< std::pair<int,int> > bfacets[6];

    int_vec2D *support, *elemmarkers;
    int_vec egroups;

    double_vec *volume, *volume_old, *volume_n;
    double_vec *mass, *tmass;
    double_vec *edvoldt;
    double_vec *temperature, *plstrain, *delta_plstrain;
    double_vec *ntmp;
    double_vec *elquality;

    array_t *vel, *force;
    tensor_t *strain_rate, *strain, *stress;
    shapefn *shpdx, *shpdy, *shpdz;

    MatProps *mat;

    MarkerSet *markerset;
};

#endif
