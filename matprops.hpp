#ifndef DYNEARTHSOL3D_MATPROPS_HPP
#define DYNEARTHSOL3D_MATPROPS_HPP

#include "parameters.hpp"

class MatProps
{
public:
    MatProps(const Param& param, const Variables& var);
    ~MatProps();

    const int rheol_type;
    const int nmat;

    double bulkm(int e) const;
    double shearm(int e) const;
    double visc(int e) const;

    double rho(int e) const;
    double cp(int e) const;
    double k(int e) const;

    void plastic_props(int e, double pls,
                       double& amc, double& anphi, double& anpsi,
                       double& hardn, double& ten_max) const;

    const double visc_min;
    const double visc_max;
    const double tension_max;
    const double therm_diff_max;

    const static int rh_elastic = 1 << 0;
    const static int rh_viscous = 1 << 1;
    const static int rh_plastic = 1 << 2;
    const static int rh_plastic2d = rh_plastic | 1 << 3;
    const static int rh_maxwell = rh_elastic | rh_viscous;
    const static int rh_ep = rh_elastic | rh_plastic;
    const static int rh_ep2d = rh_elastic | rh_plastic2d;
    const static int rh_evp = rh_elastic | rh_viscous | rh_plastic;
    const static int rh_evp2d = rh_elastic | rh_viscous | rh_plastic2d;

private:

    const double_vec &rho0, &alpha;
    const double_vec &bulk_modulus, &shear_modulus;
    const double_vec &visc_exponent, &visc_coefficient, &visc_activation_energy;
    const double_vec &heat_capacity, &therm_cond;
    const double_vec &pls0, &pls1;
    const double_vec &cohesion0, &cohesion1;
    const double_vec &friction_angle0, &friction_angle1;
    const double_vec &dilation_angle0, &dilation_angle1;

    // alias to field variables in var
    // ie. var.mat.temperature == var.temperature
    const array_t &coord;
    const conn_t &connectivity;
    const double_vec &temperature;
    const tensor_t &stress;
    const tensor_t &strain_rate;
    const int_vec2D &elemmarkers;

    void plastic_weakening(int e, double pls,
                           double &cohesion, double &friction_angle,
                           double &dilation_angle, double &hardening) const;
};


#endif
