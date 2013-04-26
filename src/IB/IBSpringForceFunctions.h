// Filename: IBSpringForceFunctions.h
// Created on 14 Jul 2004 by Boyce Griffith
//
// Copyright (c) 2002-2013, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of New York University nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef included_IBSpringForceFunctions
#define included_IBSpringForceFunctions

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{

/*!
 * \brief Typedef specifying the spring force function API.
 *
 * \return               The (undirected) "tension" force generated by the spring.
 * \param R              The displacement between the "master" and "slave" nodes associated with the spring.
 * \param params         Constitutive parameters.
 * \param rst            The resting length of the spring.
 * \param lag_mastr_idx  The Lagrangian index of the "master" node associated with the spring.
 * \param lag_slave_idx  The Lagrangian index of the "slave" node associated with the spring.
 */
typedef
double
(*SpringForceFcnPtr)(
    double R,
    const double* params,
    int lag_mastr_idx,
    int lag_slave_idx);

/*!
 * \brief Typedef specifying the spring force derivative function API.
 *
 * \return               The derivative with respect to R of the (undirected) "tension" force generated by the spring.
 * \param R              The displacement between the "master" and "slave" nodes associated with the spring.
 * \param params         Constitutive parameters.
 * \param lag_mastr_idx  The Lagrangian index of the "master" node associated with the spring.
 * \param lag_slave_idx  The Lagrangian index of the "slave" node associated with the spring.
 */
typedef
double
(*SpringForceDerivFcnPtr)(
    double R,
    const double* params,
    int lag_mastr_idx,
    int lag_slave_idx);

/*!
 * \brief Function to compute the (undirected) "tension" force generated by a
 * Hookean spring with either a zero or a non-zero resting length.
 *
 * Consider a single spring, and let \f$ k \f$ be the index of the "master" node
 * and \f$ l \f$ the index of the "slave" node.  The forces generated are \f[
 *
 *      \vec{F}_k = \kappa_{k,l} \left( \|\vec{X}_{l} - \vec{X}_{k}\| - r_{k,l} \right) \frac{\vec{X}_{l} - \vec{X}_{k}}{\|\vec{X}_{l} - \vec{X}_{k}\|},
 *
 * \f] and \f[
 *
 *      \vec{F}_l = \kappa_{k,l} \left( \|\vec{X}_{l} - \vec{X}_{k}\| - r_{k,l} \right) \frac{\vec{X}_{k} - \vec{X}_{l}}{\|\vec{X}_{l} - \vec{X}_{k}\|} = - \vec{F}_k,
 *
 * \f] where \f$ \kappa_{k,l} \f$ is the stiffness of the spring connecting
 * nodes \f$ k \f$ and \f$ l \f$, and where \f$ r_{k,l} \f$ is the associated
 * resting length of the spring.
 *
 * This function computes the undirected spring "tension" force \f$ T_{k,l} \f$
 * associated with these forces, \f[
 *
 *      T_{k,l} = \kappa_{k,l} \left( \|\vec{X}_{l} - \vec{X}_{k}\| - r_{k,l} \right).
 *
 * \f] \f$ T_{k,l} \f$ is used to compute the \em directed spring forces by
 * IBStandardForceGen via as \f[
 *
 *      \vec{F}_k = T_{k,l} \frac{\vec{X}_{l} - \vec{X}_{k}}{\|\vec{X}_{l} - \vec{X}_{k}\|}
 *
 * \f] and \f[
 *
 *      \vec{F}_l = T_{k,l} \frac{\vec{X}_{k} - \vec{X}_{l}}{\|\vec{X}_{k} - \vec{X}_{l}\|}.
 *
 * \f]
 *
 * \return               The (undirected) "tension" force generated by the spring.
 * \param R              The displacement between the "master" and "slave" nodes associated with the spring.
 * \param params         Constitutive parameters.
 * \param lag_mastr_idx  The Lagrangian index of the "master" node associated with the spring.
 * \param lag_slave_idx  The Lagrangian index of the "slave" node associated with the spring.
 *
 * \note This is the default force generation function employed by class
 * IBStandardForceGen.  It is associated with \a force_fcn_idx \a 0.  Users may
 * override this default value with any function that implements the interface
 * required by IBStandardForceGen::registerSpringForceFunction().
 */
inline double
default_spring_force(
    double R,
    const double* params,
    int /*lag_mastr_idx*/,
    int /*lag_slave_idx*/)
{
    return params[0]*(R-params[1]);
}// default_spring_force

/*!
 * \brief Function to compute the derivative with respect to R of the tension
 * force generated by a Hookean spring with either a zero or a non-zero resting
 * length.
 */
inline double
default_spring_force_deriv(
    double /*R*/,
    const double* params,
    int /*lag_mastr_idx*/,
    int /*lag_slave_idx*/)
{
    return params[0];
}// default_spring_force_deriv

}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/IBSpringForceFunctions.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBSpringForceFunctions
