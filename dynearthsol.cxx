#include <iostream>

#ifdef USE_OMP
#include <omp.h>
#endif

#include "constants.hpp"
#include "parameters.hpp"
#include "bc.hpp"
#include "binaryio.hpp"
#include "fields.hpp"
#include "geometry.hpp"
#include "ic.hpp"
#include "input.hpp"
#include "matprops.hpp"
#include "markerset.hpp"
#include "mesh.hpp"
#include "output.hpp"
#include "phasechanges.hpp"
#include "remeshing.hpp"
#include "rheology.hpp"


void init(const Param& param, Variables& var)
{
    std::cout << "Initializing mesh and field data...\n";

    create_new_mesh(param, var);
    create_boundary_flags(var);
    create_boundary_nodes(var);
    create_boundary_facets(var);
    create_support(var);
    create_elem_groups(var);
    create_elemmarkers(param, var);
    create_markers(param, var);

    allocate_variables(param, var);

    compute_volume(*var.coord, *var.connectivity, *var.volume);
    *var.volume_old = *var.volume;
    compute_mass(param, var.egroups, *var.connectivity, *var.volume, *var.mat,
                 var.max_vbc_val, *var.volume_n, *var.mass, *var.tmass);
    compute_shape_fn(*var.coord, *var.connectivity, *var.volume, var.egroups,
                     *var.shpdx, *var.shpdy, *var.shpdz);

    apply_vbcs(param, var, *var.vel);
    // temperature should be init'd before stress and strain
    initial_temperature(param, var, *var.temperature);
    initial_stress_state(param, var, *var.stress, *var.strain, var.compensation_pressure);
    initial_weak_zone(param, var, *var.plstrain);
}


void restart(const Param& param, Variables& var)
{
    std::cout << "Initializing mesh and field data from checkpoints...\n";

    /* Reading info file */
    {
        char filename[256];
        std::snprintf(filename, 255, "%s.info", param.sim.restarting_from_modelname.c_str());
        std::FILE *f = std::fopen(filename, "r");
        int frame, steps, nnode, nelem, nseg;
        while (1) {
            int n = std::fscanf(f, "%d %d %*f %*f %*f %d %d %d\n",
                                &frame, &steps, &nnode, &nelem, &nseg);
            if (n != 5) {
                std::cerr << "Error: reading info file: " << filename << '\n';
                std::exit(2);
            }
            if (frame == param.sim.restarting_from_frame)
                break;
        }

        var.steps = steps;
        var.nnode = nnode;
        var.nelem = nelem;
        var.nseg = nseg;

        std::fclose(f);
    }

    char filename_save[256];
    std::snprintf(filename_save, 255, "%s.save.%06d",
                  param.sim.restarting_from_modelname.c_str(), param.sim.restarting_from_frame);
    BinaryInput bin_save(filename_save);
    std::cout << "  Reading " << filename_save << "...\n";

    char filename_chkpt[256];
    std::snprintf(filename_chkpt, 255, "%s.chkpt.%06d",
                  param.sim.restarting_from_modelname.c_str(), param.sim.restarting_from_frame);
    BinaryInput bin_chkpt(filename_chkpt);
    std::cout << "  Reading " << filename_chkpt << "...\n";

    //
    // Following the same procedure in init()
    //

    // Reading mesh, replacing create_new_mesh()
    {
        var.coord = new array_t(var.nnode);
        bin_save.read_array(*var.coord, "coordinate");
        var.connectivity = new conn_t(var.nelem);
        bin_save.read_array(*var.connectivity, "connectivity");

        var.segment = new segment_t(var.nseg);
        bin_chkpt.read_array(*var.segment, "segment");
        var.segflag = new segflag_t(var.nseg);
        bin_chkpt.read_array(*var.segflag, "segflag");
        // Note: regattr is not needed for restarting
        // var.regattr = new regattr_t(var.nelem);
        // bin_chkpt.read_array(*var.regattr, "regattr", var.nelem);
    }

    create_boundary_flags(var);
    create_boundary_nodes(var);
    create_boundary_facets(var);
    create_support(var);
    create_elem_groups(var);
    create_elemmarkers(param, var);

    // Replacing create_markers()
    var.markerset = new MarkerSet(param, var, bin_chkpt);

    allocate_variables(param, var);

    compute_volume(*var.coord, *var.connectivity, *var.volume);
    bin_chkpt.read_array(*var.volume_old, "volume_old");
    compute_mass(param, var.egroups, *var.connectivity, *var.volume, *var.mat,
                 var.max_vbc_val, *var.volume_n, *var.mass, *var.tmass);
    compute_shape_fn(*var.coord, *var.connectivity, *var.volume, var.egroups,
                     *var.shpdx, *var.shpdy, *var.shpdz);

    apply_vbcs(param, var, *var.vel);

    // Initializing field variables
    {
        bin_save.read_array(*var.vel, "velocity");
        bin_save.read_array(*var.temperature, "temperature");
        bin_save.read_array(*var.strain_rate, "strain-rate");
        bin_save.read_array(*var.strain, "strain");
        bin_save.read_array(*var.stress, "stress");
        bin_save.read_array(*var.plstrain, "plastic strain");
    }

    // Misc. items
    {
        double_vec tmp(2);
        bin_chkpt.read_array(tmp, "time compensation_pressure");
        var.time = tmp[0];
        var.compensation_pressure = tmp[1];

        // the next two fields are not needed for restarting
        bin_save.read_array(*var.elquality, "mesh quality");
        bin_save.read_array(*var.force, "force");
    }
}


void update_mesh(const Param& param, Variables& var)
{
    update_coordinate(var, *var.coord);
    surface_processes(param, var, *var.coord);

    var.volume->swap(*var.volume_old);
    compute_volume(*var.coord, *var.connectivity, *var.volume);
    compute_mass(param, var.egroups, *var.connectivity, *var.volume, *var.mat,
                 var.max_vbc_val, *var.volume_n, *var.mass, *var.tmass);
    compute_shape_fn(*var.coord, *var.connectivity, *var.volume, var.egroups,
                     *var.shpdx, *var.shpdy, *var.shpdz);
}


int main(int argc, const char* argv[])
{
    double start_time = 0;
#ifdef USE_OMP
    start_time = omp_get_wtime();
#endif

    //
    // read command line
    //
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " config_file\n";
        std::cout << "       " << argv[0] << " -h or --help\n";
        return -1;
    }

    Param param;
    get_input_parameters(argv[1], param);

    //
    // run simulation
    //
    static Variables var; // declared as static to silence valgrind's memory leak detection
    Output output(param, start_time,
                  (param.sim.is_restarting) ? param.sim.restarting_from_frame : 0);
    var.time = 0;
    var.steps = 0;

    if (param.control.characteristic_speed == 0)
        var.max_vbc_val = find_max_vbc(param.bc);
    else
        var.max_vbc_val = param.control.characteristic_speed;

    if (! param.sim.is_restarting) {
        init(param, var);
    }
    else {
        restart(param, var);
    }

    var.dt = compute_dt(param, var);
    output.write(var, false);

    double starting_time = var.time; // var.time & var.steps might be set in restart()
    double starting_step = var.steps;
    int next_regular_frame = 1;  // excluding frames due to output_during_remeshing

    std::cout << "Starting simulation...\n";
    do {
        var.steps ++;
        var.time += var.dt;

        if (param.control.has_thermal_diffusion)
            update_temperature(param, var, *var.temperature, *var.ntmp);

        update_strain_rate(var, *var.strain_rate);
        compute_dvoldt(var, *var.ntmp);
        compute_edvoldt(var, *var.ntmp, *var.edvoldt);
        update_stress(var, *var.stress, *var.strain, *var.plstrain,  *var.delta_plstrain, *var.strain_rate);
        update_force(param, var, *var.force);
        update_velocity(var, *var.vel);
        apply_vbcs(param, var, *var.vel);
        update_mesh(param, var);

        // elastic stress/strain are objective (frame-indifferent)
        if (var.mat->rheol_type & MatProps::rh_elastic)
            rotate_stress(var, *var.stress, *var.strain);

        // dt computation is expensive, and dt only changes slowly
        // don't have to do it every time step
        if (var.steps % 10 == 0) var.dt = compute_dt(param, var);

        // ditto for phase changes
        if (var.steps % 10 == 0) phase_changes(param, var, *var.markerset, *var.elemmarkers);

        if (param.sim.output_averaged_fields)
            output.average_fields(var);

	if ((! param.sim.output_averaged_fields || (var.steps % param.sim.output_averaged_fields == 0)) &&
            // When output_averaged_fields in turned on, the output cannot be
            // done at arbitrary time steps.
            (((var.steps - starting_step) == next_regular_frame * param.sim.output_step_interval) ||
             ((var.time - starting_time) > next_regular_frame * param.sim.output_time_interval_in_yr * YEAR2SEC)) ) {

            if (next_regular_frame % param.sim.checkpoint_frame_interval == 0)
                output.write_checkpoint(var);

            output.write(var);

            next_regular_frame ++;
        }

        if (var.steps % param.mesh.quality_check_step_interval == 0) {
            int quality_is_bad, bad_quality_index;
            quality_is_bad = bad_mesh_quality(param, var, bad_quality_index);
            if (quality_is_bad) {

                if (param.sim.has_output_during_remeshing) {
                    output.write(var, false);
                }

                remesh(param, var, quality_is_bad);

                if (param.sim.has_output_during_remeshing) {
                    output.write(var, false);
                }
            }
        }

    } while (var.steps < param.sim.max_steps && var.time <= param.sim.max_time_in_yr * YEAR2SEC);

    std::cout << "Ending simulation.\n";
    return 0;
}
