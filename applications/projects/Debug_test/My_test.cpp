/******************************************************************************
*       SOFA, Simulation Open-Framework Architecture, version 1.0 RC 1        *
*                (c) 2006-2011 INRIA, USTL, UJF, CNRS, MGH                    *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU General Public License as published by the Free  *
* Software Foundation; either version 2 of the License, or (at your option)   *
* any later version.                                                          *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    *
* more details.                                                               *
*                                                                             *
* You should have received a copy of the GNU General Public License along     *
* with this program; if not, write to the Free Software Foundation, Inc., 51  *
* Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.                   *
*******************************************************************************
*                            SOFA :: Applications                             *
*                                                                             *
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/


#include <gtest/gtest.h>
#include "../Standard_test/Sofa_test.h"
#include <sofa/component/init.h>
#include "../../tutorials/objectCreator/ObjectCreator.h"
#include <sofa/simulation/graph/DAGSimulation.h>

#include <sofa/component/odesolver/EulerImplicitSolver.h>
#include <sofa/component/linearsolver/CGLinearSolver.h>
#include <sofa/component/linearsolver/FullVector.h>
#include <sofa/simulation/common/MechanicalVisitor.h>
#include <sofa/simulation/common/GetVectorVisitor.h>
#include <sofa/simulation/common/GetAssembledSizeVisitor.h>

using std::cout;
using std::cerr;
using std::endl;


namespace sofa
{
using namespace simulation;
using namespace modeling;
using namespace component;
typedef component::linearsolver::CGLinearSolver<component::linearsolver::GraphScatteredMatrix, component::linearsolver::GraphScatteredVector> CGLinearSolver;
typedef component::linearsolver::FullVector<SReal> FullVector;


struct Solver_test : public Sofa_test<SReal>
{


    /** Initialize the sofa library and create the root of the scene graph
      */
    void initSofa()
    {
        setSimulation(new graph::DAGSimulation());
        sofa::component::init();
        root = modeling::newRoot();
    }

    /** Initialize the scene graph and compute the size of the assembled vectors
      */
    void initScene()
    {
        sofa::simulation::getSimulation()->init(root.get());

        GetAssembledSizeVisitor getSizeVisitor;
        root->execute(getSizeVisitor);
        xsize = getSizeVisitor.positionSize();
        vsize = getSizeVisitor.velocitySize();
    }

    /** size of assembled position vector
    @pre the scene must be initialized
    @sa initScene()
    */
    unsigned xSize() const { return xsize; }

    /** size of assembled velocity or force vector
    @pre the scene must be initialized
    @sa initScene()
    */
    unsigned vSize() const { return vsize; }


    /** fill the vector with the positions.
      @param x the position vector
      */
    void getAssembledPositionVector( FullVector* x )
    {
        x->resize(xSize());
        GetVectorVisitor getVec( core::MechanicalParams::defaultInstance(), x, core::VecCoordId::position());
        getRoot()->execute(getVec);
    }

    /** fill the vector with the velocities.
      @param v the velocity vector
      */
    void getAssembledVelocityVector( FullVector* v )
    {
        v->resize(vSize());
        GetVectorVisitor getVec( core::MechanicalParams::defaultInstance(), v, core::VecDerivId::velocity());
        getRoot()->execute(getVec);
    }

    /** Get the root of the scene graph
      @pre The scene must be initialized
      @sa initScene()
      */
    Node::SPtr getRoot(){ return root; }


private:
    Node::SPtr root;
    unsigned xsize; ///< size of assembled position vector, computed in initScene()
    unsigned vsize; ///< size of assembled velocity vector, computed in initScene()

};


struct EulerImplicit_test_2_particles_to_equilibrium : public Solver_test
{
    EulerImplicit_test_2_particles_to_equilibrium()
    {
        //*******
        initSofa();
        //*******
        // begin create scene under the root node

        modeling::addNew<odesolver::EulerImplicitSolver>(getRoot(),"odesolver" );
        modeling::addNew<CGLinearSolver>(getRoot(),"linearsolver");

        Node::SPtr string = massSpringString(
                    getRoot(),
                    0,0,0, // first endpoint
                    1,0,0, // second endpoint
                    2,     // number of particles
                    2.0,    // total mass
                    1000.0,   // stiffness
                    0.1    // damping ratio
                    );
        FixedConstraint3::SPtr fixed = modeling::addNew<FixedConstraint3>(string,"fixedConstraint");
        fixed->addConstraint(0);

        // end create scene
        //*********
        initScene();
        //*********

        FullVector x0, x1, v0, v1;
        getAssembledPositionVector(&x0);
//        cerr<<"My_test, initial positions : " << x0 << endl;
        getAssembledVelocityVector(&v0);
//        cerr<<"My_test, initial velocities: " << v0 << endl;

        Real dx, dv;
        unsigned n=0;
        const unsigned nMax=100;
        const double  precision = 1.e-4;
        do {
            sofa::simulation::getSimulation()->animate(getRoot().get(),1.0);

            getAssembledPositionVector(&x1);
//            cerr<<"My_test, new positions : " << x1 << endl;
            getAssembledVelocityVector(&v1);
//            cerr<<"My_test, new velocities: " << v1 << endl;

            dx = this->vectorCompare(x0,x1);
            dv = this->vectorCompare(v0,v1);
            x0 = x1;
            v0 = v1;
            n++;

        } while( (dx>1.e-4 || dv>1.e-4) && n<nMax );

        // test convergence
        if( n==nMax )
            ADD_FAILURE() << "Solver test has not converged in " << nMax << " iterations, precision = " << precision << endl
                          <<" previous x = " << x0 << endl
                          <<" current x  = " << x1 << endl
                          <<" previous v = " << v0 << endl
                          <<" current v  = " << v1 << endl;

        // test position of the second particle
        Vec3d expected(0,-1.00981,0), actual( x0[3],x0[4],x0[5]);
        if( vectorCompare(expected,actual)>precision )
            ADD_FAILURE() << "Solver test has not converged to the expected position" <<
                             " expected: " << expected << endl <<
                             " actual " << actual << endl;

    }

};

TEST_F( EulerImplicit_test_2_particles_to_equilibrium, myEulerImplicit_test_2particles )
{
}

}// namespace sofa




///// This is a starting point for a test fixture. Copy-paste it and uncomment it to create yours.
//namespace sofa
//{

//struct My_test : public Sofa_test<>
//{
//};

//TEST_F( My_test, my_test )
//{
//}

//}// namespace sofa





