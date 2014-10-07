
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef USE_OMP
#include <omp.h>
#endif

#ifdef THREED

#define TETLIBRARY
#include "tetgen/tetgen.h"
#undef TETLIBRARY

#endif // THREED

#define REAL double
#define VOID void
#define ANSI_DECLARATORS
#include "triangle/triangle.h"
#undef REAL
#undef VOID
#undef ANSI_DECLARATORS


#include "constants.hpp"
#include "parameters.hpp"
#include "sortindex.hpp"
#include "utils.hpp"
#include "mesh.hpp"
#include "markerset.hpp"


namespace { // anonymous namespace

void set_verbosity_str(std::string &verbosity, int meshing_verbosity)
{
    switch (meshing_verbosity) {
    case -1:
        verbosity = "Q";
        break;
    case 0:
        verbosity = "";
        break;
    case 1:
        verbosity = "V";
        break;
    case 2:
        verbosity = "VV";
        break;
    case 3:
        verbosity = "VVV";
        break;
    default:
        verbosity = "";
        break;
    }
}


void set_volume_str(std::string &vol, double max_volume)
{
    vol.clear();
    if (max_volume > 0) {
        vol += 'a';
        vol += std::to_string((long double)max_volume);
    }
}


void set_2d_quality_str(std::string &quality, double min_angle)
{
    quality.clear();
    if (min_angle > 0) {
        quality += 'q';
        quality += std::to_string((long double)min_angle);
    }
}


void triangulate_polygon
(double min_angle, double max_area,
 int meshing_verbosity,
 int npoints, int nsegments,
 const double *points, const int *segments, const int *segflags,
 const int nregions, const double *regionattributes,
 int *noutpoints, int *ntriangles, int *noutsegments,
 double **outpoints, int **triangles,
 int **outsegments, int **outsegflags, double **outregattr)
{
    char options[255];
    triangulateio in, out;

    std::string verbosity, vol, quality;
    set_verbosity_str(verbosity, meshing_verbosity);
    set_volume_str(vol, max_area);
    set_2d_quality_str(quality, min_angle);

    if( nregions > 0 )
        std::sprintf(options, "%s%spjz%sA", verbosity.c_str(), quality.c_str(), vol.c_str());
    else
        std::sprintf(options, "%s%spjz%s", verbosity.c_str(), quality.c_str(), vol.c_str());

    if( meshing_verbosity >= 0 )
        std::cout << "The meshing option is: " << options << '\n';

    in.pointlist = const_cast<double*>(points);
    in.pointattributelist = NULL;
    in.pointmarkerlist = NULL;
    in.numberofpoints = npoints;
    in.numberofpointattributes = 0;

    in.trianglelist = NULL;
    in.triangleattributelist = NULL;
    in.trianglearealist = NULL;
    in.numberoftriangles = 0;
    in.numberofcorners = 3;
    in.numberoftriangleattributes = 0;

    in.segmentlist = const_cast<int*>(segments);
    in.segmentmarkerlist = const_cast<int*>(segflags);
    in.numberofsegments = nsegments;

    in.holelist = NULL;
    in.numberofholes = 0;

    in.numberofregions = nregions;
    if( nregions > 0 )
        in.regionlist = const_cast<double*>(regionattributes);
    else
        in.regionlist = NULL;

    out.pointlist = NULL;
    out.pointattributelist = NULL;
    out.pointmarkerlist = NULL;
    out.trianglelist = NULL;
    out.triangleattributelist = NULL;
    out.neighborlist = NULL;
    out.segmentlist = NULL;
    out.segmentmarkerlist = NULL;
    out.edgelist = NULL;
    out.edgemarkerlist = NULL;

    /*******************************/
    triangulate(options, &in, &out, NULL);
    /*******************************/

    *noutpoints = out.numberofpoints;
    *outpoints = out.pointlist;

    *ntriangles = out.numberoftriangles;
    *triangles = out.trianglelist;

    *noutsegments = out.numberofsegments;
    *outsegments = out.segmentlist;
    *outsegflags = out.segmentmarkerlist;
    *outregattr = out.triangleattributelist;

    trifree(out.pointmarkerlist);
}


void set_3d_quality_str(std::string &quality, double max_ratio,
                        double min_dihedral_angle, double max_dihedral_angle)
{
    quality.clear();
    if (max_ratio > 0) {
        quality += 'q';
        quality += std::to_string((long double)max_ratio);
        quality += "qq";
        quality += std::to_string((long double)min_dihedral_angle);
        quality += "qqq";
        quality += std::to_string((long double)max_dihedral_angle);
    }
}


void tetrahedralize_polyhedron
(double max_ratio, double min_dihedral_angle, double max_volume,
 int vertex_per_polygon, int meshing_verbosity, int optlevel,
 int npoints, int nsegments,
 const double *points, const int *segments, const int *segflags,
 const int nregions, const double *regionattributes,
 int *noutpoints, int *ntriangles, int *noutsegments,
 double **outpoints, int **triangles,
 int **outsegments, int **outsegflags, double **outregattr)
{
#ifdef THREED
    //
    // Setting Tetgen options.
    //
    char options[255];
    double max_dihedral_angle = 180 - 3 * min_dihedral_angle;

    std::string verbosity, vol, quality;
    set_verbosity_str(verbosity, meshing_verbosity);
    set_volume_str(vol, max_volume);
    set_3d_quality_str(quality, max_ratio, min_dihedral_angle, max_dihedral_angle);

    if( nregions > 0 )
        std::sprintf(options, "%s%s%spzs%dA", verbosity.c_str(), quality.c_str(), vol.c_str(), optlevel);
    else
        std::sprintf(options, "%s%s%spzs%d", verbosity.c_str(), quality.c_str(), vol.c_str(), optlevel);

    if( meshing_verbosity >= 0 )
        std::cout << "The meshing option is: " << options << '\n';

    //
    // Setting input arrays to tetgen
    //

    // NOTE: all tetgenio pointers will need to be
    // reset to NULL to prevent double-free error
    tetgenio in;
    in.pointlist = const_cast<double*>(points);
    in.numberofpoints = npoints;

    tetgenio::polygon *polys = new tetgenio::polygon[nsegments];
    for (int i=0; i<nsegments; ++i) {
        polys[i].vertexlist = const_cast<int*>(&segments[i*vertex_per_polygon]);
        polys[i].numberofvertices = vertex_per_polygon;
    }

    tetgenio::facet *fl = new tetgenio::facet[nsegments];
    for (int i=0; i<nsegments; ++i) {
        fl[i].polygonlist = &polys[i];
        fl[i].numberofpolygons = 1;
        fl[i].holelist = NULL;
        fl[i].numberofholes = 0;
    }

    in.facetlist = fl;
    in.facetmarkerlist = const_cast<int*>(segflags);
    in.numberoffacets = nsegments;

    in.holelist = NULL;
    in.numberofholes = 0;

    in.numberofregions = nregions;
    if( nregions > 0 )
        in.regionlist = const_cast<double*>(regionattributes);
    else
        in.regionlist = NULL;

    tetgenio out;
    /*******************************/
    tetrahedralize(options, &in, &out, NULL, NULL);
    /*******************************/

    // the destructor of tetgenio will free any non-NULL pointer
    // set in.pointers to NULL to prevent double-free
    in.pointlist = NULL;
    in.facetmarkerlist = NULL;
    in.facetlist = NULL;
    in.regionlist = NULL;
    delete [] polys;
    delete [] fl;

    *noutpoints = out.numberofpoints;
    *outpoints = out.pointlist;
    out.pointlist = NULL;

    *ntriangles = out.numberoftetrahedra;
    *triangles = out.tetrahedronlist;
    out.tetrahedronlist = NULL;

    *noutsegments = out.numberoftrifaces;
    *outsegments = out.trifacelist;
    *outsegflags = out.trifacemarkerlist;
    *outregattr = out.tetrahedronattributelist;
    out.trifacelist = NULL;
    out.trifacemarkerlist = NULL;
    out.tetrahedronattributelist = NULL;

#endif
}


void points_to_mesh(const Param &param, Variables &var,
                    int npoints, const double *points,
                    int n_init_segments, const int *init_segments, const int *init_segflags,
                    int nregions, const double *regattr,
                    double max_elem_size, int vertex_per_polygon)
{
    double *pcoord, *pregattr;
    int *pconnectivity, *psegment, *psegflag;

    points_to_new_mesh(param.mesh, npoints, points,
                       n_init_segments, init_segments, init_segflags,
                       nregions, regattr,
                       max_elem_size, vertex_per_polygon,
                       var.nnode, var.nelem, var.nseg,
                       pcoord, pconnectivity, psegment, psegflag, pregattr);

    var.coord = new array_t(pcoord, var.nnode);
    var.connectivity = new conn_t(pconnectivity, var.nelem);
    var.segment = new segment_t(psegment, var.nseg);
    var.segflag = new segflag_t(psegflag, var.nseg);
    var.regattr = new regattr_t(pregattr, var.nelem);
}


void new_mesh_uniform_resolution(const Param& param, Variables& var)
{
    int npoints = 4 * (NDIMS - 1); // 2D:4;  3D:8
    double *points = new double[npoints*NDIMS];

    int n_init_segments = 2 * NDIMS; // 2D:4;  3D:6
    int n_segment_nodes = 2 * (NDIMS - 1); // 2D:2; 3D:4
    int *init_segments = new int[n_init_segments*n_segment_nodes];
    int *init_segflags = new int[n_init_segments];

    const int attr_ndata = NDIMS+2;
    const int nregions = (param.ic.mattype_option == 0) ? param.mat.nmat : 0;
    double *regattr = new double[nregions*attr_ndata]; // each region has (NDIMS+2) data fields: x, (y,) z, region marker (mat type), and volume.

    double elem_size;  // size of a typical element
    int vertex_per_polygon = 4;

#ifndef THREED
    {
	/* Define 4 corner points of the rectangle, with this order:
         *            BOUNDZ1
         *          0 ------- 3
         *  BOUNDX0 |         | BOUNDX1
         *          1 ------- 2
         *            BOUNDZ0
         */
        // corner 0
        points[0] = 0;
	points[1] = 0;
        // corner 1
	points[2] = 0;
	points[3] = -param.mesh.zlength;
        // corner 2
	points[4] = param.mesh.xlength;
	points[5] = -param.mesh.zlength;
        // corner 3
	points[6] = param.mesh.xlength;
	points[7] = 0;
	
	for (int i=0; i<n_init_segments; ++i) {
            // 0th node of the i-th segment
	    init_segments[2*i] = i;
            // 1st node of the i-th segment
	    init_segments[2*i+1] = i+1;
	}
	// the 1st node of the last segment is connected back to the 0th node
	init_segments[2*n_init_segments-1] = 0;

        // boundary flags (see definition in constants.hpp)
        init_segflags[0] = BOUNDX0;
        init_segflags[1] = BOUNDZ0;
        init_segflags[2] = BOUNDX1;
        init_segflags[3] = BOUNDZ1;

        elem_size = 1.5 * param.mesh.resolution * param.mesh.resolution;

        // Need to modify when nregions > 1.
        for (int i = 0; i < nregions; i++) {
            regattr[i * attr_ndata] = 0.5*param.mesh.xlength;
            regattr[i * attr_ndata + 1] = -0.5*param.mesh.zlength;
            regattr[i * attr_ndata + 2] = 0;
            regattr[i * attr_ndata + 3] = -1;
        }
    }
#else
    {
	/* Define 8 corner points of the box, with this order:
         *         4 ------- 7
         *        /         /|
         *       /         / 6
         *      0 ------- 3 /
         *      |         |/
         *      1 ------- 2
         *
         * Cut-out diagram with boundary flag:
         *             4 ------- 7
         *             | BOUNDZ1 |
         *   4 ------- 0 ------- 3 ------- 7 ------- 4
         *   | BOUNDX0 | BOUNDY0 | BOUNDX1 | BOUNDY1 |
         *   5 ------- 1 ------- 2 ------- 6 ------- 5
         *             | BOUNDZ0 |
         *             5 ------- 6
         */

        // corner 0
        points[0] = 0;
	points[1] = 0;
	points[2] = 0;
        // corner 1
	points[3] = 0;
	points[4] = 0;
	points[5] = -param.mesh.zlength;
        // corner 2
	points[6] = param.mesh.xlength;
	points[7] = 0;
	points[8] = -param.mesh.zlength;
        // corner 3
	points[9] = param.mesh.xlength;
	points[10] = 0;
	points[11] = 0;
        // corner 4
	points[12] = 0;
	points[13] = param.mesh.ylength;
	points[14] = 0;
        // corner 5
	points[15] = 0;
	points[16] = param.mesh.ylength;
	points[17] = -param.mesh.zlength;
        // corner 6
	points[18] = param.mesh.xlength;
	points[19] = param.mesh.ylength;
	points[20] = -param.mesh.zlength;
        // corner 7
	points[21] = param.mesh.xlength;
	points[22] = param.mesh.ylength;
	points[23] = 0;

        // BOUNDX0
        init_segments[0] = 0;
        init_segments[1] = 1;
        init_segments[2] = 5;
        init_segments[3] = 4;
        // BOUNDY0
        init_segments[4] = 0;
        init_segments[5] = 3;
        init_segments[6] = 2;
        init_segments[7] = 1;
        // BOUNDZ0
        init_segments[8] = 1;
        init_segments[9] = 2;
        init_segments[10] = 6;
        init_segments[11] = 5;
        // BOUNDX1
        init_segments[12] = 3;
        init_segments[13] = 7;
        init_segments[14] = 6;
        init_segments[15] = 2;
        // BOUNDY1
        init_segments[16] = 7;
        init_segments[17] = 4;
        init_segments[18] = 5;
        init_segments[19] = 6;
        // BOUNDZ1
        init_segments[20] = 0;
        init_segments[21] = 4;
        init_segments[22] = 7;
        init_segments[23] = 3;

        // boundary flags (see definition in constants.hpp)
        init_segflags[0] = BOUNDX0;
        init_segflags[1] = BOUNDY0;
        init_segflags[2] = BOUNDZ0;
        init_segflags[3] = BOUNDX1;
        init_segflags[4] = BOUNDY1;
        init_segflags[5] = BOUNDZ1;

        elem_size = 0.7 * param.mesh.resolution
            * param.mesh.resolution * param.mesh.resolution;

        // Need to modify when nregions > 1. Using .poly file might be better.
        for (int i = 0; i < nregions; i++) {
            regattr[i * attr_ndata] = 0.5*param.mesh.xlength;
            regattr[i * attr_ndata + 1] = 0.5*param.mesh.ylength;
            regattr[i * attr_ndata + 2] = -0.5*param.mesh.zlength;
            regattr[i * attr_ndata + 3] = 0;
            regattr[i * attr_ndata + 4] = -1;
        }
    }
#endif

    points_to_mesh(param, var, npoints, points,
                   n_init_segments, init_segments, init_segflags, nregions, regattr,
                   elem_size, vertex_per_polygon);

    delete [] points;
    delete [] init_segments;
    delete [] init_segflags;
    delete [] regattr;
}


void new_mesh_refined_zone(const Param& param, Variables& var)
{
    const Mesh& m = param.mesh;

    // To prevent the meshing library giving us a regular grid, the nodes
    // will be shifted randomly by a small distance
    const double shift_factor = 0.1;

    // typical distance between nodes in the refined zone
    const double d = param.mesh.resolution / std::sqrt(2);

    // adjust the bounds of the refined zone so that nodes are not on the boundary
    double x0, x1, y0, y1, z0, z1;
    double dx, dy, dz;
    int nx, ny, nz;
    x0 = std::max(m.refined_zonex.first, d / m.xlength);
    x1 = std::min(m.refined_zonex.second, 1 - d / m.xlength);
    nx = m.xlength * (x1 - x0) / d + 1;
    dx = m.xlength * (x1 - x0) / nx;
    z0 = std::max(m.refined_zonez.first, d / m.zlength);
    z1 = std::min(m.refined_zonez.second, 1 - d / m.zlength);
    nz = m.zlength * (z1 - z0) / d + 1;
    dz = m.zlength * (z1 - z0) / (nz - 1);

    int npoints;
#ifndef THREED
    npoints = nx * nz + 4 * (NDIMS - 1);
#else
    y0 = std::max(m.refined_zoney.first, d / m.ylength);
    y1 = std::min(m.refined_zoney.second, 1 - d / m.ylength);
    ny = m.ylength * (y1 - y0) / d + 1;
    dy = m.ylength * (y1 - y0) / (ny - 1);
    npoints = nx * ny * nz + 4 * (NDIMS - 1);
#endif

    double *points = new double[npoints*NDIMS];

    int n_init_segments = 2 * NDIMS; // 2D:4;  3D:6
    int n_segment_nodes = 2 * (NDIMS - 1); // 2D:2; 3D:4
    int *init_segments = new int[n_init_segments*n_segment_nodes];
    int *init_segflags = new int[n_init_segments];

    const int attr_ndata = NDIMS+2;
    const int nregions = (param.ic.mattype_option == 0) ? param.mat.nmat : 0;
    double *regattr = new double[nregions*attr_ndata]; // each region has (NDIMS+2) data fields: x, (y,) z, region marker (mat type), and volume.

    double max_elem_size;
    int vertex_per_polygon = 4;

#ifndef THREED
    {
	/* Define 4 corner points of the rectangle, with this order:
         *            BOUNDZ1
         *          0 ------- 3
         *  BOUNDX0 |         | BOUNDX1
         *          1 ------- 2
         *            BOUNDZ0
         */
        // corner 0
        points[0] = 0;
	points[1] = 0;
        // corner 1
	points[2] = 0;
	points[3] = -param.mesh.zlength;
        // corner 2
	points[4] = param.mesh.xlength;
	points[5] = -param.mesh.zlength;
        // corner 3
	points[6] = param.mesh.xlength;
	points[7] = 0;

        // add refined nodes
        int n = 8;
        for (int i=0; i<nx; ++i) {
            for (int k=0; k<nz; ++k) {
                double rx = drand48() - 0.5;
                double rz = drand48() - 0.5;
                points[n  ] = x0 * m.xlength + (i + shift_factor*rx) * dx;
                points[n+1] = (1-z0) * -m.zlength + (k + shift_factor*rz) * dz;
                n += NDIMS;
            }
        }

	for (int i=0; i<n_init_segments; ++i) {
            // 0th node of the i-th segment
	    init_segments[2*i] = i;
            // 1st node of the i-th segment
	    init_segments[2*i+1] = i+1;
	}
	// the 1st node of the last segment is connected back to the 0th node
	init_segments[2*n_init_segments-1] = 0;

        // boundary flags (see definition in constants.hpp)
        init_segflags[0] = BOUNDX0;
        init_segflags[1] = BOUNDZ0;
        init_segflags[2] = BOUNDX1;
        init_segflags[3] = BOUNDZ1;

        max_elem_size = 1.5 * param.mesh.resolution * param.mesh.resolution
            * param.mesh.largest_size;

        // Need to modify when nregions > 1.
        for (int i = 0; i < nregions; i++) {
            regattr[i * attr_ndata] = 0.5*param.mesh.xlength;
            regattr[i * attr_ndata + 1] = -0.5*param.mesh.zlength;
            regattr[i * attr_ndata + 2] = 0;
            regattr[i * attr_ndata + 3] = -1;
        }
    }
#else
    {
	/* Define 8 corner points of the box, with this order:
         *         4 ------- 7
         *        /         /|
         *       /         / 6
         *      0 ------- 3 /
         *      |         |/
         *      1 ------- 2
         *
         * Cut-out diagram with boundary flag:
         *             4 ------- 7
         *             | BOUNDZ1 |
         *   4 ------- 0 ------- 3 ------- 7 ------- 4
         *   | BOUNDX0 | BOUNDY0 | BOUNDX1 | BOUNDY1 |
         *   5 ------- 1 ------- 2 ------- 6 ------- 5
         *             | BOUNDZ0 |
         *             5 ------- 6
         */

        // corner 0
        points[0] = 0;
	points[1] = 0;
	points[2] = 0;
        // corner 1
	points[3] = 0;
	points[4] = 0;
	points[5] = -param.mesh.zlength;
        // corner 2
	points[6] = param.mesh.xlength;
	points[7] = 0;
	points[8] = -param.mesh.zlength;
        // corner 3
	points[9] = param.mesh.xlength;
	points[10] = 0;
	points[11] = 0;
        // corner 4
	points[12] = 0;
	points[13] = param.mesh.ylength;
	points[14] = 0;
        // corner 5
	points[15] = 0;
	points[16] = param.mesh.ylength;
	points[17] = -param.mesh.zlength;
        // corner 6
	points[18] = param.mesh.xlength;
	points[19] = param.mesh.ylength;
	points[20] = -param.mesh.zlength;
        // corner 7
	points[21] = param.mesh.xlength;
	points[22] = param.mesh.ylength;
	points[23] = 0;

        // add refined nodes
        int n = 24;
        for (int i=0; i<nx; ++i) {
            for (int j=0; j<ny; ++j) {
                for (int k=0; k<nz; ++k) {
                    double rx = drand48() - 0.5;
                    double ry = drand48() - 0.5;
                    double rz = drand48() - 0.5;
                    points[n  ] = x0 * m.xlength + (i + shift_factor*rx) * dx;
                    points[n+1] = y0 * m.ylength + (j + shift_factor*ry) * dy;
                    points[n+2] = (1-z0) * -m.zlength + (k + shift_factor*rz) * dz;
                    n += NDIMS;
                }
            }
        }

        // BOUNDX0
        init_segments[0] = 0;
        init_segments[1] = 1;
        init_segments[2] = 5;
        init_segments[3] = 4;
        // BOUNDY0
        init_segments[4] = 0;
        init_segments[5] = 3;
        init_segments[6] = 2;
        init_segments[7] = 1;
        // BOUNDZ0
        init_segments[8] = 1;
        init_segments[9] = 2;
        init_segments[10] = 6;
        init_segments[11] = 5;
        // BOUNDX1
        init_segments[12] = 3;
        init_segments[13] = 7;
        init_segments[14] = 6;
        init_segments[15] = 2;
        // BOUNDY1
        init_segments[16] = 7;
        init_segments[17] = 4;
        init_segments[18] = 5;
        init_segments[19] = 6;
        // BOUNDZ1
        init_segments[20] = 0;
        init_segments[21] = 4;
        init_segments[22] = 7;
        init_segments[23] = 3;

        // boundary flags (see definition in constants.hpp)
        init_segflags[0] = BOUNDX0;
        init_segflags[1] = BOUNDY0;
        init_segflags[2] = BOUNDZ0;
        init_segflags[3] = BOUNDX1;
        init_segflags[4] = BOUNDY1;
        init_segflags[5] = BOUNDZ1;

        max_elem_size = 0.7 * param.mesh.resolution * param.mesh.resolution
            * param.mesh.resolution * param.mesh.largest_size;

        // Need to modify when nregions > 1.
        for (int i = 0; i < nregions; i++) {
            regattr[i * attr_ndata] = 0.5*param.mesh.xlength;
            regattr[i * attr_ndata + 1] = 0.5*param.mesh.ylength;
            regattr[i * attr_ndata + 2] = -0.5*param.mesh.zlength;
            regattr[i * attr_ndata + 3] = 0;
            regattr[i * attr_ndata + 4] = -1;
        }
    }
#endif

    points_to_mesh(param, var, npoints, points,
                   n_init_segments, init_segments, init_segflags, nregions, regattr,
                   max_elem_size, vertex_per_polygon);

    delete [] points;
    delete [] init_segments;
    delete [] init_segflags;
    delete [] regattr;
}


void my_fgets(char *buffer, int size, std::FILE *fp,
              int &lineno, const std::string &filename)
{
    char *s;
    while (1) {
        ++ lineno;
        s = std::fgets(buffer, size, fp);
        if (! s) {
            std::cerr << "Error: reading line " << lineno
                      << " of '" << filename << "'\n";
            std::exit(2);
        }

        // check for blank lines and comments
        if (buffer[0] != '\n' && buffer[0] != '#') break;
    }
}


void new_mesh_from_polyfile(const Param& param, Variables& var)
{
    /* The format specifiction for the poly file can be found in:
     *   2D:  http://www.cs.cmu.edu/~quake/triangle.poly.html
     *   3D:  http://wias-berlin.de/software/tetgen/fformats.poly.html
     *
     * Note that the poly file in 3D has a more complicated format.
     */

    double max_elem_size;
#ifdef THREED
    max_elem_size = 0.7 * param.mesh.resolution
        * param.mesh.resolution * param.mesh.resolution;
#else
    max_elem_size = 1.5 * param.mesh.resolution * param.mesh.resolution;
#endif

    std::FILE *fp = std::fopen(param.mesh.poly_filename.c_str(), "r");
    if (! fp) {
        std::cerr << "Error: Cannot open poly_filename '" << param.mesh.poly_filename << "'\n";
        std::exit(2);
    }

    int lineno = 0;
    int n;
    char buffer[255];

    // get header of node list
    int npoints;
    {
        my_fgets(buffer, 255, fp, lineno, param.mesh.poly_filename);

        int dim, nattr, nbdrym;
        n = std::sscanf(buffer, "%d %d %d %d", &npoints, &dim, &nattr, &nbdrym);
        if (n != 4) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }

        if (dim != NDIMS ||
            nattr != 0 ||
            nbdrym != 0) {
            std::cerr << "Error: unsupported value in line " << lineno
                      << " of '" << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
    }

    // get node list
    double *points = new double[npoints * NDIMS];
    for (int i=0; i<npoints; i++) {
        my_fgets(buffer, 255, fp, lineno, param.mesh.poly_filename);

        int junk;
        double *x = &points[i*NDIMS];
#ifdef THREED
        n = std::sscanf(buffer, "%d %lf %lf %lf", &junk, x, x+1, x+2);
#else
        n = std::sscanf(buffer, "%d %lf %lf", &junk, x, x+1);
#endif
        if (n != NDIMS+1) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
    }

    // get header of segment (facet) list
    int n_init_segments;
    {
        my_fgets(buffer, 255, fp, lineno, param.mesh.poly_filename);

        int has_bdryflag;
        n = std::sscanf(buffer, "%d %d", &n_init_segments, &has_bdryflag);
        if (n != 2) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }

        if (has_bdryflag != 1) {
            std::cerr << "Error: unsupported value in line " << lineno
                      << " of '" << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
    }

    // get segment (facet) list
    int *init_segments = new int[n_init_segments * NODES_PER_FACET];
    int *init_segflags = new int[n_init_segments];
    for (int i=0; i<n_init_segments; i++) {
        my_fgets(buffer, 255, fp, lineno, param.mesh.poly_filename);

        int *x = &init_segments[i*NODES_PER_FACET];

        int junk;
#ifdef THREED
        int npolygons, nholes, bdryflag;
        n = std::sscanf(buffer, "%d %d %d", &npolygons, &nholes, &bdryflag);
        if (n != 3) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
        if (npolygons != 1 ||
            nholes != 0) {
            std::cerr << "Error: unsupported value in line " << lineno
                      << " of '" << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }

        init_segflags[i] = bdryflag;

        my_fgets(buffer, 255, fp, lineno, param.mesh.poly_filename);

        int nvertex;
        n = std::sscanf(buffer, "%d %d %d %d", &nvertex, x, x+1, x+2);
        if (nvertex != NODES_PER_FACET) {
            std::cerr << "Error: unsupported value in line " << lineno
                      << " of '" << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
        if (n != NODES_PER_FACET+1) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
#else
        n = std::sscanf(buffer, "%d %d %d %d", &junk, x, x+1, init_segflags+i);
        if (n != NODES_PER_FACET+2) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
#endif
    }


    // get header of hole list
    {
        my_fgets(buffer, 255, fp, lineno, param.mesh.poly_filename);

        int nholes;
        n = std::sscanf(buffer, "%d", &nholes);
        if (n != 1) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }

        if (nholes != 0) {
            std::cerr << "Error: unsupported value in line " << lineno
                      << " of '" << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
    }

    // get header of region list
    int nregions;
    {
        my_fgets(buffer, 255, fp, lineno, param.mesh.poly_filename);

        n = std::sscanf(buffer, "%d", &nregions);
        if (n != 1) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
        
        if (param.ic.mattype_option == 0 && nregions != param.mat.nmat) {
            std::cerr << "Error: Number of regions should be exactly 'mat.num_materials' but a different value is given in line " << lineno
                      << " of '" << param.mesh.poly_filename << "'\n";
            std::exit(1);
        }
    }

    // get region list
    double *regattr = new double[nregions * (NDIMS+2)]; // each region has 5 data fields: x, (y,) z, region marker (mat type), and volume.
    for (int i=0; i<nregions; i++) {
        my_fgets(buffer, 255, fp, lineno, param.mesh.poly_filename);

        int junk;
        double *x = &regattr[i*(NDIMS+2)];
#ifdef THREED
        n = std::sscanf(buffer, "%d %lf %lf %lf %lf %lf", &junk, x, x+1, x+2, x+3, x+4);
#else
        n = std::sscanf(buffer, "%d %lf %lf %lf %lf", &junk, x, x+1, x+2, x+3);
#endif
        if (n != NDIMS+3) {
            std::cerr << "Error: parsing line " << lineno << " of '"
                      << param.mesh.poly_filename << "'. "<<NDIMS+3<<" values should be given but only "<<n<<" found.\n";
            std::exit(1);
        }

        if ( x[NDIMS] < 0 || x[NDIMS] >= param.mat.nmat ) {
            std::cerr << "Error: "<<NDIMS+2<<"-th value in line "<<lineno<<" should be >=0 and < "<<param.mat.nmat<<" (=mat.num_materials) but is "<<x[NDIMS]<<"\n";
            std::cerr << "Note that this parameter is directly used as the index of mat. prop. arrays.\n";
            std::exit(1);
        }
    }

    points_to_mesh(param, var, npoints, points,
                   n_init_segments, init_segments, init_segflags, nregions, regattr,
                   max_elem_size, NODES_PER_FACET);

    delete [] points;
    delete [] init_segments;
    delete [] init_segflags;
    delete [] regattr;
}

}


void points_to_new_mesh(const Mesh &mesh, int npoints, const double *points,
                        int n_init_segments, const int *init_segments, const int *init_segflags,
                        int n_regions, const double *regattr,
                        double max_elem_size, int vertex_per_polygon,
                        int &nnode, int &nelem, int &nseg, double *&pcoord,
                        int *&pconnectivity, int *&psegment, int *&psegflag, double *&pregattr)
{
#ifdef THREED

    tetrahedralize_polyhedron(mesh.max_ratio,
                              mesh.min_tet_angle, max_elem_size,
                              vertex_per_polygon,
                              mesh.meshing_verbosity,
                              mesh.tetgen_optlevel,
                              npoints, n_init_segments, points,
                              init_segments, init_segflags,
                              n_regions, regattr,
                              &nnode, &nelem, &nseg,
                              &pcoord, &pconnectivity,
                              &psegment, &psegflag, &pregattr);

#else

    triangulate_polygon(mesh.min_angle, max_elem_size,
                        mesh.meshing_verbosity,
                        npoints, n_init_segments, points,
                        init_segments, init_segflags,
                        n_regions, regattr,
                        &nnode, &nelem, &nseg,
                        &pcoord, &pconnectivity,
                        &psegment, &psegflag, &pregattr);

#endif

    if (nelem <= 0) {
#ifdef THREED
        std::cerr << "Error: tetrahedralization failed\n";
#else
        std::cerr << "Error: triangulation failed\n";
#endif
        std::exit(10);
    }

}


void points_to_new_surface(const Mesh &mesh, int npoints, const double *points,
                           int n_init_segments, const int *init_segments, const int *init_segflags,
                           int n_regions, const double *regattr,
                           double max_elem_size, int vertex_per_polygon,
                           int &nnode, int &nelem, int &nseg, double *&pcoord,
                           int *&pconnectivity, int *&psegment, int *&psegflag, double *&pregattr)
{
#ifdef THREED
    /* For triangulation of boundary surfaces in 3D */

    triangulate_polygon(mesh.min_angle, max_elem_size,
                        mesh.meshing_verbosity,
                        npoints, n_init_segments, points,
                        init_segments, init_segflags,
                        n_regions, regattr,
                        &nnode, &nelem, &nseg,
                        &pcoord, &pconnectivity,
                        &psegment, &psegflag, &pregattr);

    if (nelem <= 0) {
        std::cerr << "Error: surface triangulation failed\n";
        std::exit(10);
    }
#endif
}


void renumbering_mesh(const Param& param, array_t &coord, conn_t &connectivity, segment_t &segment)
{
    /* Renumbering nodes and elements to enhance cache coherance and better parallel performace. */

    const int nnode = coord.size();
    const int nelem = connectivity.size();
    const int nseg = segment.size();

    //
    // sort coordinate of nodes and element centers
    //

    //
    double_vec wn(nnode);
    double aspect_ratio_factor = 1e-6 * param.mesh.xlength / param.mesh.zlength;
    for(int i=0; i<nnode; i++) {
        wn[i] = coord[i][0] - aspect_ratio_factor * coord[i][NDIMS-1];
    }

    double_vec we(nelem);
    for(int i=0; i<nelem; i++) {
        const int *conn = connectivity[i];
        we[i] = wn[conn[0]] + wn[conn[1]]
#ifdef THREED
            + wn[conn[2]]
#endif
            + wn[conn[NODES_PER_ELEM-1]];
    }

    // arrays to store the result of sorting
    std::vector<std::size_t> nd_idx(nnode);
    std::vector<std::size_t> el_idx(nelem);
    sortindex(wn, nd_idx);
    sortindex(we, el_idx);

    //
    // renumbering
    //
    array_t coord2(nnode);
    for(int i=0; i<nnode; i++) {
        int n = nd_idx[i];
        for(int j=0; j<NDIMS; j++)
            coord2[i][j] = coord[n][j];
    }
    coord.steal_ref(coord2);

    conn_t conn2(nelem);
    for(int i=0; i<nelem; i++) {
        int n = el_idx[i];
        for(int j=0; j<NODES_PER_ELEM; j++) {
            int k = connectivity[n][j];
            conn2[i][j] = std::find(nd_idx.begin(), nd_idx.end(), k) - nd_idx.begin();
        }
    }
    connectivity.steal_ref(conn2);

    segment_t seg2(nseg);
    for(int i=0; i<nseg; i++) {
        for(int j=0; j<NDIMS; j++) {
            int k = segment[i][j];
            seg2[i][j] = std::find(nd_idx.begin(), nd_idx.end(), k) - nd_idx.begin();
        }
    }
    segment.steal_ref(seg2);
}


void create_boundary_flags2(uint_vec &bcflag, int nseg,
                            const int *psegment, const int *psegflag)
{
    for (int i=0; i<nseg; ++i) {
        uint flag = static_cast<uint>(psegflag[i]);
        const int *n = psegment + i * NODES_PER_FACET;
        for (int j=0; j<NODES_PER_FACET; ++j) {
            bcflag[n[j]] |= flag;
        }
    }
}


void create_boundary_flags(Variables& var)
{
    // allocate and init to 0
    var.bcflag = new uint_vec(var.nnode);

    create_boundary_flags2(*var.bcflag, var.segment->size(),
                           var.segment->data(), var.segflag->data());
}


void create_boundary_nodes(Variables& var)
{
    /* var.bnodes[i] contains a list of nodes on the i-th boundary.
     * (See constants.hpp for the order of boundaries.)
     */
    for (std::size_t i=0; i<var.bcflag->size(); ++i) {
        uint f = (*var.bcflag)[i];
        for (int j=0; j<6; ++j) {
            if (f & bdry[j]) {
                // this node belongs to a boundary
                (var.bnodes[j]).push_back(i);
            }
        }
    }

    // for (int j=0; j<6; ++j) {
    //     std::cout << "boundary " << j << '\n';
    //     print(std::cout, var.bnodes[j]);
    //     std::cout << '\n';
    // }
}


void create_boundary_facets(Variables& var)
{
    /* var.bfacets[i] contains a list of facets (or segments in 2D)
     * on the i-th boundary. (See constants.hpp for the order of boundaries.)
     */
    for (int e=0; e<var.nelem; ++e) {
        const int *conn = (*var.connectivity)[e];
        for (int i=0; i<FACETS_PER_ELEM; ++i) {
            // set all bits to 1
            uint flag = BOUNDX0 | BOUNDX1 | BOUNDY0 | BOUNDY1 | BOUNDZ0 | BOUNDZ1;
            for (int j=0; j<NODES_PER_FACET; ++j) {
                // find common flags
                int n = NODE_OF_FACET[i][j];
                flag &= (*var.bcflag)[conn[n]];
            }
            if (flag) {
                // this facet belongs to a boundary
                int n = bdry_order.find(flag)->second;
                var.bfacets[n].push_back(std::make_pair(e, i));
            }
        }
    }

    // for (int n=0; n<6; ++n) {
    //     std::cout << "boundary facet " << n << ":\n";
    //     print(std::cout, var.bfacets[n]);
    //     std::cout << '\n';
    //     for (int i=0; i<var.bfacets[n].size(); ++i) {
    //         int e = var.bfacets[n][i].first;
    //         int f = var.bfacets[n][i].second;
    //         const int *conn = (*var.connectivity)[e];
    //         std::cout << i << ", " << e << ":";
    //         for (int j=0; j<NODES_PER_FACET; ++j) {
    //             std::cout << " " << conn[NODE_OF_FACET[f][j]];
    //         }
    //         std::cout << '\n';
    //     }
    //     std::cout << '\n';
    // }
}


void create_support(Variables& var)
{
    var.support = new std::vector<int_vec>(var.nnode);

    // create the inverse mapping of connectivity
    for (int e=0; e<var.nelem; ++e) {
        const int *conn = (*var.connectivity)[e];
        for (int i=0; i<NODES_PER_ELEM; ++i) {
            (*var.support)[conn[i]].push_back(e);
        }
    }
    // std::cout << "support:\n";
    // print(std::cout, *var.support);
    // std::cout << "\n";
}


void create_elem_groups(Variables& var)
{
    var.egroups.clear();

#ifdef USE_OMP

    /* T: # of openmp threads
     *
     * Decompose the mesh into 2T bands.
     * The band is ordered as: 0, 1, 2, ...., 2T-2, 2T-1.
     * Band-N and Band-(N+2) will be disjoint and not sharing any nodes.
     */

    int nthreads = omp_get_max_threads();
    int ngroups = 2 * nthreads;
    int el_per_group = var.nelem / ngroups;

    for(int i=0; i<ngroups; i++)
        var.egroups.push_back(i*el_per_group);
    var.egroups.push_back(var.nelem);

#else

    // Not using openmp, only need one group for all elements
    var.egroups.push_back(0);
    var.egroups.push_back(var.nelem);

#endif

    // print(std::cout, var.egroups);
}


void create_elemmarkers(const Param& param, Variables& var)
{
    var.elemmarkers = new int_vec2D( var.nelem, std::vector<int>(param.mat.nmat, 0) );

}


void create_markers(const Param& param, Variables& var)
{
    var.markerset = new MarkerSet( param, var );
}


void create_new_mesh(const Param& param, Variables& var)
{
    switch (param.mesh.meshing_option) {
    case 1:
        new_mesh_uniform_resolution(param, var);
        break;
    case 2:
        new_mesh_refined_zone(param, var);
        break;
    case 90:
        new_mesh_from_polyfile(param, var);
        break;
    default:
        std::cout << "Error: unknown meshing option: " << param.mesh.meshing_option << '\n';
        std::exit(1);
    }

    renumbering_mesh(param, *var.coord, *var.connectivity, *var.segment);

    // std::cout << "segment:\n";
    // print(std::cout, *var.segment);
    // std::cout << '\n';
    // std::cout << "segflag:\n";
    // print(std::cout, *var.segflag);
    // std::cout << '\n';
}


double** elem_center(const array_t &coord, const conn_t &connectivity)
{
    /* Returns the centroid of the elements.
     * Note: center[0] == tmp
     * The caller is responsible to delete [] center[0] and center!
     */
    int nelem = connectivity.size();
    double *tmp = new double[nelem*NDIMS];
    double **center = new double*[nelem];
    #pragma omp parallel for default(none)          \
        shared(nelem, tmp, coord, connectivity, center)
    for(int e=0; e<nelem; e++) {
        const int* conn = connectivity[e];
        center[e] = tmp + e*NDIMS;
        for(int d=0; d<NDIMS; d++) {
            double sum = 0;
            for(int k=0; k<NODES_PER_ELEM; k++) {
                sum += coord[conn[k]][d];
            }
            center[e][d] = sum / NODES_PER_ELEM;
        }
    }
    return center;
}


