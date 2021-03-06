#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "parameters.hpp"
#include "matprops.hpp"
#include "utils.hpp"
#include "input.hpp"

static void declare_parameters(po::options_description &cfg,
                               Param &p)
{
    /* To have a new input parameter declared as such in parameters.h,
     *
     *     struct SectionType {
     *         type name;
     *     }
     *     struct Param {
     *         SectionType section;
     *     }
     *
     * add this line:
     *
     *     ("section.name", po::value<type>(&p.section.name), "help string")
     *
     */

    cfg.add_options()
        ("sim.modelname", po::value<std::string>(&p.sim.modelname),
         "Prefix for the output files")

        ("sim.max_steps", po::value<int>(&p.sim.max_steps),
         "Max. number of time steps")
        ("sim.max_time_in_yr", po::value<double>(&p.sim.max_time_in_yr),
         "Max. time (in years)")
        ("sim.output_step_interval", po::value<int>(&p.sim.output_step_interval),
         "Output step interval")
        ("sim.output_time_interval_in_yr", po::value<double>(&p.sim.output_time_interval_in_yr),
         "Output time interval (in years)")

        ("sim.checkpoint_frame_interval", po::value<int>(&p.sim.checkpoint_frame_interval)->default_value(10),
         "How frequent to write checkpoint file (used for restarting simulation)?")
        ("sim.restarting_from_modelname", po::value<std::string>(&p.sim.restarting_from_modelname),
         "Prefix for the checkpoint and save files for restarting.")
        ("sim.restarting_from_frame", po::value<int>(&p.sim.restarting_from_frame),
         "Which output frame to read for restarting?")
        ("sim.is_restarting", po::value<bool>(&p.sim.is_restarting)->default_value(false),
         "Restarting from previous checkpoint file?")

        ("sim.has_marker_output", po::value<bool>(&p.sim.has_marker_output)->default_value(false),
         "Output marker coordinate and material?")
        ("sim.has_output_during_remeshing", po::value<bool>(&p.sim.has_output_during_remeshing)->default_value(false),
         "Output immediately before and after remeshing?")
        ("sim.output_averaged_fields", po::value<int>(&p.sim.output_averaged_fields)->default_value(1),
         "Output time-averaged (smoothed) field variables or not. These fields are: velocity, strain rate, and stress.\n"
         "0: no, output instaneous fields. The velocity and strain-rate might oscillate temporally.\n"
         "1: yes, output field variables averaged over mesh.quality_check_step_interval time steps.\n"
         "N: (integer N > 2) yes, output field variables averaged over N time steps. The value of N is strongly related to the value of mesh.quality_check_step_interval, which must be a multiple of N.\n")
        ;

    cfg.add_options()
        ("mesh.meshing_option", po::value<int>(&p.mesh.meshing_option)->default_value(1),
         "How to create the new mesh?\n"
         "1: rectangular box with roughly uniform resolution\n"
         "2: rectangular box with rectangular zone of refined resolution\n"
         "90: read the bounding polygon from a .poly file"
         )
        ("mesh.meshing_verbosity", po::value<int>(&p.mesh.meshing_verbosity)->default_value(-1),
         "Output verbose during mesh/remeshing. -1 for no output.")
        ("mesh.tetgen_optlevel", po::value<int>(&p.mesh.tetgen_optlevel)->default_value(3),
         "Optimization level for tetgen. 0: no optimization; 1: multiple edge filps; 2: 1 & free vertex deletion; 3: 2 & new vertex insertion. High optimization level could slow down the speed of mesh generation. For 3D only.")

        ("mesh.xlength", po::value<double>(&p.mesh.xlength)->required(),
         "Length of x (in meters)")
        ("mesh.ylength", po::value<double>(&p.mesh.ylength)->required(),
         "Length of y (in meters), for 3D only")
        ("mesh.zlength", po::value<double>(&p.mesh.zlength)->required(),
         "Length of z (in meters)")

        ("mesh.resolution", po::value<double>(&p.mesh.resolution)->required(),
         "Spatial resolution (in meters)")
        ("mesh.smallest_size", po::value<double>(&p.mesh.smallest_size)->default_value(0.01),
         "The size of smallest element relative to an element with typical resolution")
        ("mesh.largest_size", po::value<double>(&p.mesh.largest_size)->default_value(30),
         "The size of largest element relative to an element with typical resolution")
        // for 2D only
        ("mesh.min_angle", po::value<double>(&p.mesh.min_angle)->default_value(32.),
         "Min. angle of all triangles (in degrees), for 2D only")
        // for 3D only
        ("mesh.min_tet_angle", po::value<double>(&p.mesh.min_tet_angle)->default_value(22.),
         "Min. dihedral angle of all tetrahedra (in degrees), for 3D only")
        ("mesh.max_ratio", po::value<double>(&p.mesh.max_ratio)->default_value(2.),
         "Max. radius / length ratio of all tetrahedra, for 3D only")

        // for meshing_option = 2 only
        /* read the value as string, then parse as two numbers later */
        ("mesh.refined_zonex", po::value<std::string>()->default_value("[0.4, 0,6]"),
         "Refining portion of xlength ([d0,d1]; 0<=d0<=d1<=1), for meshing_option=2 only")
        ("mesh.refined_zoney", po::value<std::string>()->default_value("[0.4, 0.6]"),
         "Refining portion of ylength ([d0,d1]; 0<=d0<=d1<=1), for meshing_option=2 only, for 3D only")
        ("mesh.refined_zonez", po::value<std::string>()->default_value("[0.8, 1]"),
         "Refining portion of zlength ([d0,d1]; 0<=d0<=d1<=1), for meshing_option=2 only")

        // for meshing_option = 90 only
        ("mesh.poly_filename", po::value<std::string>(&p.mesh.poly_filename)->default_value("mesh.poly"),
         "Filename of the input polygon, for meshing_option=90 only.\n"
         "The file format is described in\n"
         "http://www.cs.cmu.edu/~quake/triangle.poly.html (2D) and\n"
         "http://wias-berlin.de/software/tetgen/fformats.poly.html (3D).\n"
         "Limitation: no point attributes, no boundary marker for points, no holes,\n"
         "no regional attributes, and only triangles can be used for facets (3D).\n"
         "Users still need to provide these parameters: mesh.xlength, mesh.ylength, and mesh.zlength.")

        ("mesh.quality_check_step_interval", po::value<int>(&p.mesh.quality_check_step_interval)->default_value(100),
         "How often to check mesh quality?")
        ("mesh.min_quality", po::value<double>(&p.mesh.min_quality)->default_value(0.4),
         "Min. mesh quality before remeshing (between 0 and 1)")

        ("mesh.remeshing_option", po::value<int>(&p.mesh.remeshing_option)->default_value(0),
         "How to deal with the boundaries during remeshing?\n"
         " 0: no modification on any boundary.\n"
         " 1: move all bottom nodes to initial depth, other boundaries are intact.\n"
         " 2: create a new bottom boundary at the initial depth, other boundaries are intact.\n"
         "10: no modification on any boundary, except small boundary segments might get merged.\n"
         "11: move all bottom nodes to initial depth, other boundaries are intact, small boundary segments"
         "    might get merged.\n")
        ;

    cfg.add_options()
        ("markers.init_marker_option", po::value<int>(&p.markers.init_marker_option)->default_value(1),
         "How to generate markers?\n"
         "1: randomly distributed markers.\n"
         "2: regularly spaced markers.\n")
        ("markers.markers_per_element", po::value<int>(&p.markers.markers_per_element)->default_value(4),
         "Number of markers per element. Used when init_marker_option=1.")
        ("markers.init_marker_spacing", po::value<double>(&p.markers.init_marker_spacing)->default_value(0.3),
         "Spacing of markers (in unit of mesh.resolution). Used when init_marker_option=2.")
        ;

    cfg.add_options()
        ("control.gravity", po::value<double>(&p.control.gravity)->default_value(10),
         "Magnitude of the gravity (in m/s^2)")

        ("control.characteristic_speed",
         po::value<double>(&p.control.characteristic_speed)->default_value(0),
         "Characteristic tectonic speed (in m/s). "
         "It is used to estimate the size of stable time step. "
         "If it is 0, its value is inferred from the imposed boundary velocity. "
         "Set it to other value if the imposed boundary velocity is 0 everywhere.")

        ("control.is_quasi_static", po::value<bool>(&p.control.is_quasi_static)->default_value(true),
         "Is the simulation quasi-static or dynamic? If quasi-static, inertial scaling and strong damping is applied.\n")
        ("control.dt_fraction", po::value<double>(&p.control.dt_fraction)->default_value(1.0),
         "Take dt as a fraction of max. stable time step size (0-1).\n")
        ("control.inertial_scaling", po::value<double>(&p.control.inertial_scaling)->default_value(1e5),
         "Scaling factor for inertial (a large number)")
        ("control.damping_factor", po::value<double>(&p.control.damping_factor)->default_value(0.8),
         "A factor for force damping (0-1)")

        ("control.ref_pressure_option", po::value<int>(&p.control.ref_pressure_option)->default_value(0),
         "How to define reference pressure? 0: using density of the 0-th element to compute lithostatic pressure; 1: computing rerence pressure from the PREM model.")

        ("control.surface_process_option", po::value<int>(&p.control.surface_process_option)->default_value(0),
         "What kind of surface processes? 0: no surface processes. "
         "1: using simple diffusion to modify surface topography. "
         "101: custom function.")
        ("control.surface_diffusivity", po::value<double>(&p.control.surface_diffusivity)->default_value(1e-6),
         "Diffusition coefficient of surface topography (m^2/s)")

        ("control.has_thermal_diffusion", po::value<bool>(&p.control.has_thermal_diffusion)->default_value(true),
         "Does the model have thermal diffusion? If not, temperature is advected, but not diffused.\n")

        ;

    cfg.add_options()
        ("bc.surface_temperature", po::value<double>(&p.bc.surface_temperature)->default_value(273),
         "Surface temperature (in Kelvin)")
        ("bc.mantle_temperature", po::value<double>(&p.bc.mantle_temperature)->default_value(1600),
         "Mantle temperature (in Kelvin)")
        ("bc.has_wrinkler_foundation", po::value<bool>(&p.bc.has_wrinkler_foundation)->default_value(true),
         "Using Wrinkler foundation for the bottom boundary?")
        ("bc.wrinkler_delta_rho", po::value<double>(&p.bc.wrinkler_delta_rho)->default_value(0),
         "Excess density of the bottom Wrinkler foundation (in kg/m^3)")
        ("bc.has_water_loading", po::value<bool>(&p.bc.has_water_loading)->default_value(true),
         "Applying water loading for top boundary that is below sea level?")

        ("bc.vbc_x0", po::value<int>(&p.bc.vbc_x0)->default_value(1),
         "Type of boundary condtition for left side. Possible type is \n"
         "0: all components free;\n"
         "1: normal component fixed, shear components free;\n"
         "2: normal component free, shear components fixed at 0;\n"
         "3: normal component fixed, shear components fixed at 0;\n"
         "4: normal component free, shear component (not z) fixed, only in 3D;\n"
         "5: normal component fixed at 0, shear component (not z) fixed, only in 3D;\n")
        ("bc.vbc_x1", po::value<int>(&p.bc.vbc_x1)->default_value(1),
         "Type of boundary condtition for right side")
        ("bc.vbc_val_x0", po::value<double>(&p.bc.vbc_val_x0)->default_value(-1e-9),
         "Value of boundary condtition for left side (if velocity, unit is m/s; if stress, unit is Pa)")
        ("bc.vbc_val_x1", po::value<double>(&p.bc.vbc_val_x1)->default_value(1e-9),
         "Value of boundary condtition for right side (if velocity, unit is m/s; if stress, unit is Pa)")

        ("bc.vbc_y0", po::value<int>(&p.bc.vbc_y0)->default_value(0),
         "Type of boundary condtition for back side")
        ("bc.vbc_y1", po::value<int>(&p.bc.vbc_y1)->default_value(0),
         "Type of boundary condtition for front side")
        ("bc.vbc_val_y0", po::value<double>(&p.bc.vbc_val_y0)->default_value(0),
         "Value of boundary condtition for back side (if velocity, unit is m/s; if stress, unit is Pa)")
        ("bc.vbc_val_y1", po::value<double>(&p.bc.vbc_val_y1)->default_value(0),
         "Value of boundary condtition for front side (if velocity, unit is m/s; if stress, unit is Pa)")

        ("bc.vbc_z0", po::value<int>(&p.bc.vbc_z0)->default_value(0),
         "Type of boundary condtition for bottom side")
        ("bc.vbc_z1", po::value<int>(&p.bc.vbc_z1)->default_value(0),
         "Type of boundary condtition for top side")
        ("bc.vbc_val_z0", po::value<double>(&p.bc.vbc_val_z0)->default_value(0),
         "Value of boundary condtition for bottom side (if velocity, unit is m/s; if stress, unit is Pa)")
        ("bc.vbc_val_z1", po::value<double>(&p.bc.vbc_val_z1)->default_value(0),
         "Value of boundary condtition for top side (if velocity, unit is m/s; if stress, unit is Pa)")
        ;

    cfg.add_options()
        ("ic.mattype_option", po::value<int>(&p.ic.mattype_option)->default_value(0),
         "How to set the initial material type of markers?\n"
         "0: marker's mattype is determined by regional attribute in meshing.\n"
         "1: marker's mattype is determined by its location.\n")

        ("ic.weakzone_option", po::value<int>(&p.ic.weakzone_option)->default_value(1),
         "How to set the initial weak zone?\n"
         "0: no weak zone.\n"
         "1: planar weak zone with specified azimuth, inclination, halfwidth, min/max depth range, and center location.\n"
         "2: ellipsoidal weak zone with specified center location and semi-axes.\n")
        ("ic.weakzone_plstrain", po::value<double>(&p.ic.weakzone_plstrain)->default_value(0.1),
         "Initial plastic strain in the weak zone")
        ("ic.weakzone_azimuth", po::value<double>(&p.ic.weakzone_azimuth)->default_value(0),
         "Azimuth angle (relative to +y axis) (in degree)")
        ("ic.weakzone_inclination", po::value<double>(&p.ic.weakzone_inclination)->default_value(90),
         "Inclination angle (relative to horizontal plane) (in degree)")
        ("ic.weakzone_halfwidth", po::value<double>(&p.ic.weakzone_halfwidth)->default_value(1.5),
         "Half-width of the weak zone (in unit of mesh.resolution)")
        ("ic.weakzone_y_min", po::value<double>(&p.ic.weakzone_y_min)->default_value(0),
         "Y-direction range (between 0 and 1, in unit of mesh.ylength)")
        ("ic.weakzone_y_max", po::value<double>(&p.ic.weakzone_y_max)->default_value(1),
         "Y-direction range (between 0 and 1, in unit of mesh.ylength)")
        ("ic.weakzone_depth_min", po::value<double>(&p.ic.weakzone_depth_min)->default_value(0),
         "Depth upper (shallower) range (between 0 and 1, in unit of mesh.zlength)")
        ("ic.weakzone_depth_max", po::value<double>(&p.ic.weakzone_depth_max)->default_value(1),
         "Depth lower (deeper) range (between 0 and 1, in unit of mesh.zlength)")
        ("ic.weakzone_xcenter", po::value<double>(&p.ic.weakzone_xcenter)->default_value(0.5),
         "Location of weak zone center in x direction (between 0 and 1, in unit of mesh.xlength)")
        ("ic.weakzone_ycenter", po::value<double>(&p.ic.weakzone_ycenter)->default_value(0.5),
         "Location of weak zone center in y direction (between 0 and 1, in unit of mesh.ylength)")
        ("ic.weakzone_zcenter", po::value<double>(&p.ic.weakzone_zcenter)->default_value(0.5),
         "Location of weak zone center in z direction (between 0 and 1, in unit of mesh.zlength)")
        ("ic.weakzone_xsemi_axis", po::value<double>(&p.ic.weakzone_xsemi_axis)->default_value(1e3),
         "Length of weak zone semi-axis in x direction (in meters)")
        ("ic.weakzone_ysemi_axis", po::value<double>(&p.ic.weakzone_ysemi_axis)->default_value(1e3),
         "Length of weak zone semi-axis in y direction (in meters)")
        ("ic.weakzone_zsemi_axis", po::value<double>(&p.ic.weakzone_zsemi_axis)->default_value(1e3),
         "Length of weak zone semi-axis in z direction (in meters)")

        ("ic.oceanic_plate_age_in_yr", po::value<double>(&p.ic.oceanic_plate_age_in_yr)->default_value(60e6),
         "Age of the oceanic plate (in years), used for the temperature profile on the plate.\n")
        ;

    cfg.add_options()
        ("mat.rheology_type", po::value<std::string>()->required(),
         "Type of rheology, either 'elastic', 'viscous', 'maxwell', "
         "'elasto-plastic' (general), 'elasto-plastic2d' (plane strain formulation, 2D only), "
         "'elasto-visco-plastic' (general), or 'elasto-visco-plastic2d' (plane strain formulation, 2D only).")
        ("mat.phase_change_option", po::value<int>(&p.mat.phase_change_option)->default_value(0),
         "What kind of phase changes?\n"
         "0: no phase changes.\n"
         "101: custom phase changes.")
        ("mat.num_materials", po::value<int>(&p.mat.nmat)->default_value(1),
         "Number of material types")
        ("mat.max_viscosity", po::value<double>(&p.mat.visc_max)->default_value(1e24),
         "Max. value of viscosity (in Pa.s)")
        ("mat.min_viscosity", po::value<double>(&p.mat.visc_min)->default_value(1e18),
         "Min. value of viscosity (in Pa.s)")
        ("mat.max_tension", po::value<double>(&p.mat.tension_max)->default_value(1e9),
         "Max. value of tensile stress (in Pa)")
        ("mat.max_thermal_diffusivity", po::value<double>(&p.mat.therm_diff_max)->default_value(5e-6),
         "Max. value of thermal diffusivity (in m^2/s)")

        // these parameters need to parsed later
        ("mat.rho0", po::value<std::string>()->default_value("[3210]"),
         "Density of the materials at 0 Pa and 273 K '[d0, d1, d2, ...]' (in kg/m^3)")
        ("mat.alpha", po::value<std::string>()->default_value("[3e-5]"),
         "Volumetic thermal expansion of the materials '[d0, d1, d2, ...]' (in 1/Kelvin)")

        ("mat.bulk_modulus", po::value<std::string>()->default_value("[128.2e9]"),
         "Bulk modulus of the materials '[d0, d1, d2, ...]' (in Pa)")
        ("mat.shear_modulus", po::value<std::string>()->default_value("[80.5e9]"),
         "Shear modulus of the materials '[d0, d1, d2, ...]' (in Pa)")

        ("mat.visc_exponent", po::value<std::string>()->default_value("[3.05]"),
         "Exponents of non-linear viscosity of the materials'[d0, d1, d2, ...]'")
        ("mat.visc_coefficient", po::value<std::string>()->default_value("[1.25e-1]"),
         "Pre-exponent coefficient of non-linear viscosity of the materials '[d0, d1, d2, ...]'")
        ("mat.visc_activation_energy", po::value<std::string>()->default_value("[3.76e5]"),
         "Activation energy of non-linear viscosity of the materials '[d0, d1, d2, ...]' (in J/mol)")

        ("mat.heat_capacity", po::value<std::string>()->default_value("[1e3]"),
         "Heat capacity (isobaric) of the materials '[d0, d1, d2, ...]' (in J/kg/Kelvin)")
        ("mat.therm_cond", po::value<std::string>()->default_value("[3]"),
         "Thermal conductivity of the materials '[d0, d1, d2, ...]' (in W/m/Kelvin)")

        ("mat.pls0", po::value<std::string>()->default_value("[0]"),
         "Plastic strain of the materials where weakening starts '[d0, d1, d2, ...]' (no unit)")
        ("mat.pls1", po::value<std::string>()->default_value("[0.1]"),
         "Plastic strain of the materials where weakening saturates '[d0, d1, d2, ...]' (no unit)")
        ("mat.cohesion0", po::value<std::string>()->default_value("[4e7]"),
         "Cohesion of the materials when weakening starts '[d0, d1, d2, ...]' (in Pa)")
        ("mat.cohesion1", po::value<std::string>()->default_value("[4e6]"),
         "Cohesion of the materials when weakening saturates '[d0, d1, d2, ...]' (in Pa)")
        ("mat.friction_angle0", po::value<std::string>()->default_value("[30]"),
         "Friction angle of the materials when weakening starts '[d0, d1, d2, ...]' (in degree)")
        ("mat.friction_angle1", po::value<std::string>()->default_value("[5]"),
         "Friction angle of the materials when weakening saturates '[d0, d1, d2, ...]' (in degree)")
        ("mat.dilation_angle0", po::value<std::string>()->default_value("[0]"),
         "Dilation angle of the materials when weakening starts '[d0, d1, d2, ...]' (in degree)")
        ("mat.dilation_angle1", po::value<std::string>()->default_value("[0]"),
         "Dilation angle of the materials when weakening saturates '[d0, d1, d2, ...]' (in degree)")
        ;
}


static void read_parameters_from_file
(const char* filename,
 const po::options_description cfg,
 po::variables_map &vm)
{
    try {
        po::store(po::parse_config_file<char>(filename, cfg), vm);
        po::notify(vm);
    }
    catch (const boost::program_options::multiple_occurrences& e) {
        std::cerr << e.what() << " from option: " << e.get_option_name() << '\n';
        std::exit(1);
    }
    catch (std::exception& e) {
        std::cerr << "Error reading config_file '" << filename << "'\n";
        std::cerr << e.what() << "\n";
        std::exit(1);
    }
}


static int read_numbers(const std::string &input, double_vec &vec, int len)
{
    /* Read 'len' numbers from input.
     * The format of input must be '[n0, n1, n2]' or '[n0, n1, n2,]' (with a trailing ,),
     * for len=3.
     */

    std::istringstream stream(input);
    vec.resize(len);

    char sentinel;

    stream >> sentinel;
    if (sentinel != '[') return 1;

    for (int i=0; i<len; ++i) {
        stream >> vec[i];

        if (i == len-1) break;

        // consume ','
        char sep;
        stream >> sep;
        if (sep != ',') return 1;
    }

    stream >> sentinel;
    if (sentinel == ',') stream >> sentinel;
    if (sentinel != ']') return 1;

    if (! stream.good()) return 1;

    // success
    return 0;
}


static void get_numbers(const po::variables_map &vm, const char *name,
                        double_vec &values, int len)
{
    if ( ! vm.count(name) ) {
        std::cerr << "Error: " << name << " is not provided.\n";
        std::exit(1);
    }

    std::string str = vm[name].as<std::string>();
    int err = read_numbers(str, values, len);
    if (err) {
        std::cerr << "Error: incorrect format for " << name << ",\n"
                  << "       must be '[d0, d1, d2, ...]'\n";
        std::exit(1);
    }
}


static void validate_parameters(const po::variables_map &vm, Param &p)
{
    std::cout << "Checking consisitency of input parameters...\n";

    //
    // stopping condition and output interval are based on either model time or step
    //
    if ( ! (vm.count("sim.max_steps") || vm.count("sim.max_time_in_yr")) ) {
        std::cerr << "Must provide either sim.max_steps or sim.max_time_in_yr\n";
        std::exit(1);
    }
    if ( ! vm.count("sim.max_steps") )
        p.sim.max_steps = std::numeric_limits<int>::max();;
    if ( ! vm.count("sim.max_time_in_yr") )
        p.sim.max_time_in_yr = std::numeric_limits<double>::max();;

    if ( ! (vm.count("sim.output_step_interval") || vm.count("sim.output_time_interval_in_yr")) ) {
        std::cerr << "Must provide either sim.output_step_interval or sim.output_time_interval_in_yr\n";
        std::exit(1);
    }
    if ( ! vm.count("sim.output_step_interval") )
        p.sim.output_step_interval = std::numeric_limits<int>::max();;
    if ( ! vm.count("sim.output_time_interval_in_yr") )
        p.sim.output_time_interval_in_yr = std::numeric_limits<double>::max();;

    //
    // These parameters are required when restarting
    //
    if (p.sim.is_restarting) {
        if ( ! vm.count("sim.restarting_from_modelname") ) {
            std::cerr << "Must provide sim.restarting_from_modelname when restarting.\n";
            std::exit(1);
        }
        if ( ! vm.count("sim.restarting_from_frame") ) {
            std::cerr << "Must provide sim.restarting_from_frame when restarting.\n";
            std::exit(1);
        }
    }

    if (p.sim.output_averaged_fields == 1)
        p.sim.output_averaged_fields = p.mesh.quality_check_step_interval;
    if (p.sim.output_averaged_fields && (p.mesh.quality_check_step_interval % p.sim.output_averaged_fields) != 0) {
        std::cerr << "mesh.quality_check_step_interval must be a multiple of sim.output_averaged_fields!.\n";
        std::exit(1);
    }
    if (p.sim.output_averaged_fields && (p.sim.output_step_interval < p.sim.output_averaged_fields)) {
        std::cerr << "sim.output_step_interval is less than sim.output_averaged_fields!.\n";
        std::exit(1);
    }

    //
    // these parameters are required in mesh.meshing_option == 2
    //
    if (p.mesh.meshing_option == 2) {
        if ( ! vm.count("mesh.refined_zonex") ||
#ifdef THREED
             ! vm.count("mesh.refined_zoney") ||
#endif
             ! vm.count("mesh.refined_zonez") ) {
        std::cerr << "Must provide mesh.refined_zonex, "
#ifdef THREED
                  << "mesh.refined_zoney, "
#endif
                  << "mesh.refined_zonez.\n";
        std::exit(1);
        }

        /* get 2 numbers from the string */
        double_vec tmp;
        int err;
        std::string str;
        str = vm["mesh.refined_zonex"].as<std::string>();
        err = read_numbers(str, tmp, 2);
        if (err || tmp[0] < 0 || tmp[1] > 1 || tmp[0] > tmp[1]) {
            std::cerr << "Error: incorrect value for mesh.refine_zonex,\n"
                      << "       must in this format '[d0, d1]', 0 <= d0 <= d1 <= 1.\n";
            std::exit(1);
        }
        p.mesh.refined_zonex.first = tmp[0];
        p.mesh.refined_zonex.second = tmp[1];
#ifdef THREED
        str = vm["mesh.refined_zoney"].as<std::string>();
        err = read_numbers(str, tmp, 2);
        if (err || tmp[0] < 0 || tmp[1] > 1 || tmp[0] > tmp[1]) {
            std::cerr << "Error: incorrect value for mesh.refine_zoney,\n"
                      << "       must in this format '[d0, d1]', 0 <= d0 <= d1 <= 1.\n";
            std::exit(1);
        }
        p.mesh.refined_zoney.first = tmp[0];
        p.mesh.refined_zoney.second = tmp[1];
#endif
        str = vm["mesh.refined_zonez"].as<std::string>();
        err = read_numbers(str, tmp, 2);
        if (err || tmp[0] < 0 || tmp[1] > 1 || tmp[0] > tmp[1]) {
            std::cerr << "Error: incorrect value for mesh.refine_zonez,\n"
                      << "       must in this format '[d0, d1]', 0 <= d0 <= d1 <= 1.\n";
            std::exit(1);
        }
        p.mesh.refined_zonez.first = tmp[0];
        p.mesh.refined_zonez.second = tmp[1];
    }

    if (p.mesh.smallest_size > p.mesh.largest_size) {
        std::cerr << "Error: mesh.smallest_size is greater than mesh.largest_size.\n";
        std::exit(1);
    }


    //
    // bc
    //
    {
        if ( p.bc.has_wrinkler_foundation && p.control.gravity == 0 ) {
            p.bc.has_wrinkler_foundation = 0;
            std::cerr << "Warning: no gravity, Wrinkler foundation is turned off.\n";
        }
        if ( p.bc.has_wrinkler_foundation && p.bc.vbc_z0 != 0 ) {
            std::cerr << "Error: vbc_z0 is not 0, but Wrinkler foundation is turned on.\n";
            std::exit(1);
        }
        if ( p.bc.has_water_loading && p.control.gravity == 0 ) {
            p.bc.has_water_loading = 0;
            std::cerr << "Warning: no gravity, water loading is turned off.\n";
        }
        if ( p.bc.has_wrinkler_foundation && p.bc.vbc_z1 != 0 ) {
            std::cerr << "Error: vbc_z1 is not 0, but water loading is turned on.\n";
            std::exit(1);
        }
    }

    //
    // control
    //
    {
        if ( p.control.dt_fraction < 0 || p.control.dt_fraction > 1 ) {
            std::cerr << "Error: control.dt_fraction must be between 0 and 1.\n";
            std::exit(1);
        }
        if ( p.control.damping_factor < 0 || p.control.damping_factor > 1 ) {
            std::cerr << "Error: control.damping_factor must be between 0 and 1.\n";
            std::exit(1);
        }

    }

    //
    // marker
    //
    {
        if ( p.markers.markers_per_element <= 2 ) {
            std::cerr << "Error: markers.markers.markers_per_element must be greater than 2.\n";
            std::exit(1);
        }

    }

    //
    // material properties
    //
    {
        std::string str = vm["mat.rheology_type"].as<std::string>();
        if (str == std::string("elastic"))
            p.mat.rheol_type = MatProps::rh_elastic;
        else if (str == std::string("viscous"))
            p.mat.rheol_type = MatProps::rh_viscous;
        else if (str == std::string("maxwell"))
            p.mat.rheol_type = MatProps::rh_maxwell;
        else if (str == std::string("elasto-plastic"))
            p.mat.rheol_type = MatProps::rh_ep;
        else if (str == std::string("elasto-plastic2d"))
            p.mat.rheol_type = MatProps::rh_ep2d;
        else if (str == std::string("elasto-visco-plastic"))
            p.mat.rheol_type = MatProps::rh_evp;
        else if (str == std::string("elasto-visco-plastic2d"))
            p.mat.rheol_type = MatProps::rh_evp2d;
        else {
            std::cerr << "Error: unknown rheology: '" << str << "'\n";
            std::exit(1);
        }

#ifdef THREED
        if (p.mat.rheol_type == MatProps::rh_ep2d ||
            p.mat.rheol_type == MatProps::rh_evp2d) {
            std::cerr << "Error:  " << str << " rheology cannot be used in 3D.\n";
            std::exit(1);
        }
#endif

        if (p.mat.phase_change_option != 0 && p.mat.nmat == 1) {
            std::cerr << "Error: mat.phase_change_option is chosen but mat.num_materials is 1.\n";
            std::exit(1);
        }

        get_numbers(vm, "mat.rho0", p.mat.rho0, p.mat.nmat);
        get_numbers(vm, "mat.alpha", p.mat.alpha, p.mat.nmat);

        get_numbers(vm, "mat.bulk_modulus", p.mat.bulk_modulus, p.mat.nmat);
        get_numbers(vm, "mat.shear_modulus", p.mat.shear_modulus, p.mat.nmat);

        get_numbers(vm, "mat.visc_exponent", p.mat.visc_exponent, p.mat.nmat);
        get_numbers(vm, "mat.visc_coefficient", p.mat.visc_coefficient, p.mat.nmat);
        get_numbers(vm, "mat.visc_activation_energy", p.mat.visc_activation_energy, p.mat.nmat);

        get_numbers(vm, "mat.heat_capacity", p.mat.heat_capacity, p.mat.nmat);
        get_numbers(vm, "mat.therm_cond", p.mat.therm_cond, p.mat.nmat);

        get_numbers(vm, "mat.pls0", p.mat.pls0, p.mat.nmat);
        get_numbers(vm, "mat.pls1", p.mat.pls1, p.mat.nmat);
        get_numbers(vm, "mat.cohesion0", p.mat.cohesion0, p.mat.nmat);
        get_numbers(vm, "mat.cohesion1", p.mat.cohesion1, p.mat.nmat);
        get_numbers(vm, "mat.friction_angle0", p.mat.friction_angle0, p.mat.nmat);
        get_numbers(vm, "mat.friction_angle1", p.mat.friction_angle1, p.mat.nmat);
        get_numbers(vm, "mat.dilation_angle0", p.mat.dilation_angle0, p.mat.nmat);
        get_numbers(vm, "mat.dilation_angle1", p.mat.dilation_angle1, p.mat.nmat);
    }

}


void get_input_parameters(const char* filename, Param& p)
{
    po::options_description cfg("Config file options");
    po::variables_map vm;

    declare_parameters(cfg, p);
    // print help message
    if (std::strncmp(filename, "-h", 3) == 0 ||
        std::strncmp(filename, "--help", 7) == 0) {
        std::cout << cfg;
        std::exit(0);
    }
    read_parameters_from_file(filename, cfg, vm);
    validate_parameters(vm, p);
}
