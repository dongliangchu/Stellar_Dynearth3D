#ifndef DYNEARTHSOL3D_IC_HPP
#define DYNEARTHSOL3D_IC_HPP

double get_prem_pressure(double depth);
void initial_stress_state(const Param &param, const Variables &var,
                          tensor_t &stress, tensor_t &strain,
                          double &compensation_pressure);
void initial_weak_zone(const Param &param, const Variables &var,
                       double_vec &plstrain);
void initial_temperature(const Param &param, const Variables &var,
                         double_vec &temperature);


#endif
