/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    itkFEMElementQuadC02D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

// disable debug warnings in MS compiler
#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#include "itkFEMElementQuadC02D.h"
#include "itkFEMLoadGrav.h"
#include "itkFEMLoadEdge.h"
#include "itkFEMObjectFactory.h"
#include "itkFEMUtility.h"
#include "vnl/vnl_math.h"

namespace itk {
namespace fem {




/**
 * Constructor for class QuadC02D
 */
QuadC02D::QuadC02D(  Node::ConstPointer n1_,
          Node::ConstPointer n2_,
          Node::ConstPointer n3_,
          Node::ConstPointer n4_,
          Material::ConstPointer p_)
{
  try
  {
    /**
     * Initialize the pointers to nodes and check that
     * we were given the pointers to the right node class.
     * if the node class was incorrect a bad_cast exception is thrown
     */
    m_node[0]=&dynamic_cast<const NodeXY&>(*n1_);
    m_node[1]=&dynamic_cast<const NodeXY&>(*n2_);
    m_node[2]=&dynamic_cast<const NodeXY&>(*n3_);
    m_node[3]=&dynamic_cast<const NodeXY&>(*n4_);
    m_mat=&dynamic_cast<const MaterialStandard&>(*p_);
  }
  catch ( std::bad_cast )
  {
    throw FEMExceptionWrongClass(__FILE__,__LINE__,"QuadC02D::QuadC02D()");
  }

}




/**
 * Returns the stiffness matrix for QuadC02D element
 */
vnl_matrix<QuadC02D::Float> QuadC02D::Ke() const 
{
  vnl_matrix<Float> MatKe(8,8), I(2,2), shapeD(4,2), shapeINVD(4,2),
            J(2,2), D(3,3), B(3,8), DB(3,8);

  Float detJ;
  
  /** Gaussian integration points */
  Float pt = 1.0 / sqrt(3.0);
  Float GPoints[][2] = {{-pt, -pt}, {pt, -pt}, {pt, pt}, {-pt, pt}};

  /** Material properties matrix */
  Float disot = (m_mat->E*(1-m_mat->ni))/((1+m_mat->ni)*(1-2*m_mat->ni));
    
  D(0,0) = disot;
  D(0,1) = disot * (m_mat->ni) / (1 - m_mat->ni);
  D(0,2) = 0;

  D(1,0) = D(0,1);
  D(1,1) = disot;
  D(1,2) = 0;

  D(2,0) = 0;
  D(2,1) = 0;
  D(2,2) = disot * (1-2*m_mat->ni)/(2*(1-m_mat->ni));
  
  Float x[2];
  int i, j;


  /** Initialize stiffness matrix */
  MatKe.fill(0.0);

  /** For each integration point */
  for (int k=0; k<4; k++) {
    /** Get the k-th integration point */
    x[0] = GPoints[k][0];
    x[1] = GPoints[k][1];


    /**
     * Computes the Jacobian matrix and its determinant
     * at the k-th integration point
     */
    J = ComputeJacobianMatrixAt(x);
    detJ = JacobianMatrixDeterminant(J);

    /** Computes the inverse of the Jacobian matrix */
    I = ComputeJacobianInverse(J, detJ);

    /** Computes the shape function derivatives at integration point */
    shapeD = ComputeShapeFunctionDerivativesAt(x);

    /**
     * Computes the shape function derivatives in Cartesian coordinates
     * at integration point
     */
    shapeINVD = ComputeShapeFunctionCartDerivatives(I, shapeD);

    /** Computes the strain (B) matrix */
    B = ComputeBMatrix(shapeINVD);

    /** Computes the matrix multiplication DB */
    DB = ComputeDBMatrix(D,B);

    /**
     * Add the contribution of k-th integration point to
     * the stiffness matrix
     */

    /** For each row of the stiffness matrix */
    for (i=0; i<8; i++) {

      /** For each column of the stiffness matrix */
      for (j=0; j<8; j++) {

        /** Compute MatKe(i,j) */
        Float temp = 0;
        for (int k=0; k<3; k++) {
          temp += B[k][i] * DB[k][j];
        }

        MatKe[i][j] += (detJ * temp);
      }
    }
  }

  return MatKe;
}



/**
 * Draw the element on device context pDC.
 */
#ifdef FEM_BUILD_VISUALIZATION
void QuadC02D::Draw(CDC* pDC, Solution::ConstPointer sol) const 
{

  int x1=m_node[0]->X*DC_Scale;
  int y1=m_node[0]->Y*DC_Scale;
  
  int x2=m_node[1]->X*DC_Scale;
  int y2=m_node[1]->Y*DC_Scale;
  
  int x3=m_node[2]->X*DC_Scale;
  int y3=m_node[2]->Y*DC_Scale;
  
  int x4=m_node[3]->X*DC_Scale;
  int y4=m_node[3]->Y*DC_Scale;

  x1+=sol->GetSolutionValue(this->GetDegreeOfFreedom(0))*DC_Scale;
  y1+=sol->GetSolutionValue(this->GetDegreeOfFreedom(1))*DC_Scale;
  x2+=sol->GetSolutionValue(this->GetDegreeOfFreedom(2))*DC_Scale;
  y2+=sol->GetSolutionValue(this->GetDegreeOfFreedom(3))*DC_Scale;
  x3+=sol->GetSolutionValue(this->GetDegreeOfFreedom(4))*DC_Scale;
  y3+=sol->GetSolutionValue(this->GetDegreeOfFreedom(5))*DC_Scale;
  x4+=sol->GetSolutionValue(this->GetDegreeOfFreedom(6))*DC_Scale;
  y4+=sol->GetSolutionValue(this->GetDegreeOfFreedom(7))*DC_Scale;

  pDC->MoveTo(x1,y1);
  pDC->LineTo(x2,y2);
  pDC->LineTo(x3,y3);
  pDC->LineTo(x4,y4);
  pDC->LineTo(x1,y1);

}
#endif


/**
 * Returns a vector with global point p corresponding to
 * local point x in the cell.
 */
vnl_vector<QuadC02D::Float>
QuadC02D::ComputePositionAt(Float x[]) const
{
  vnl_vector<Float> p(2);
  
  vnl_vector<Float> shapeF = ComputeShapeFunctionsAt(x); 
  
  p[0] = (m_node[0]->X * shapeF[0])
     + (m_node[1]->X * shapeF[1])
     + (m_node[2]->X * shapeF[2])
     + (m_node[3]->X * shapeF[3]);

  p[1] = (m_node[0]->Y * shapeF[0])
     + (m_node[1]->Y * shapeF[1])
     + (m_node[2]->Y * shapeF[2])
     + (m_node[3]->Y * shapeF[3]);

  return p;
}




/**
 * Returns the Jacobian matrix at point x, which is given
 * with respect to the local coordinate system.
 */
vnl_matrix<QuadC02D::Float>
QuadC02D::ComputeJacobianMatrixAt(Float x[]) const
{
  vnl_matrix<Float> J(2,2);

  /**
   * Get the derivatives of the shape functions at given
   * point x
   */
  vnl_matrix<Float> shapeD = ComputeShapeFunctionDerivativesAt(x);

  /**
   * Compute the elements of the Jacobian matrix
   * for each coordinate of a node w.r.t. global coordinate system
   */
  J[0][0] = (shapeD[0][0] * m_node[0]->X)
        + (shapeD[1][0] * m_node[1]->X)
        + (shapeD[2][0] * m_node[2]->X)
        + (shapeD[3][0] * m_node[3]->X);

  J[0][1] = (shapeD[0][1] * m_node[0]->X)
        + (shapeD[1][1] * m_node[1]->X)
        + (shapeD[2][1] * m_node[2]->X)
        + (shapeD[3][1] * m_node[3]->X);

  J[1][0] = (shapeD[0][0] * m_node[0]->Y)
        + (shapeD[1][0] * m_node[1]->Y)
        + (shapeD[2][0] * m_node[2]->Y)
        + (shapeD[3][0] * m_node[3]->Y);
    
  J[1][1] = (shapeD[0][1] * m_node[0]->Y)
        + (shapeD[1][1] * m_node[1]->Y)
        + (shapeD[2][1] * m_node[2]->Y)
        + (shapeD[3][1] * m_node[3]->Y);

  return J;
}




/**
 * Returns a vector with the value of the shape functions
 * at point x, which is given with respect to the local
 * coordinate system.
 */
vnl_vector<QuadC02D::Float>
QuadC02D::ComputeShapeFunctionsAt(Float x[]) const
{
  /** Linear quadrilateral element has four shape functions  */
  vnl_vector<Float> shapeF(4);
  
  /**
   * Linear quadrilateral element has local coordinates
   * (-1,-1), (1,-1), (1,1), and (-1,1)
   */
  
  /** given local point x=(r,s), where -1 <= r,s <= 1 and */

  /** shape function 1: ((1 - r) * (1 - s)) / 4  (node 1) */
  shapeF[0] = (1 - x[0]) * (1 - x[1]) * .25;

  /** shape function 2: ((1 + r) * (1 - s)) / 4  (node 2) */
  shapeF[1] = (1 + x[0]) * (1 - x[1]) * .25;

  /** shape function 3: ((1 + r) * (1 + s)) / 4  (node 3) */
  shapeF[2] = (1 + x[0]) * (1 + x[1]) * .25;

  /** shape function 1: ((1 - r) * (1 + s)) / 4  (node 4) */
  shapeF[3] = (1 - x[0]) * (1 + x[1]) * .25;

  return shapeF;
}



/**
 * Return a matrix with the value of the derivatives of
 * the shape functions at point x, which is given with
 * respect to the local coordinate system.
 */
vnl_matrix<QuadC02D::Float>
QuadC02D::ComputeShapeFunctionDerivativesAt(Float x[]) const
{
  /** functions at directions r and s.  */
  vnl_matrix<Float> shapeD(4,2);

  /** Derivative w.r.t r for shape function 1 (node 1) */
  shapeD[0][0] = -(1 - x[1]) * .25;

  /** Derivative w.r.t s for shape function 1 (node 1) */
  shapeD[0][1] = -(1 - x[0]) * .25;

  /** Derivative w.r.t r for shape function 2 (node 2) */
  shapeD[1][0] = +(1 - x[1]) * .25;

  /** Derivative w.r.t s for shape function 2 (node 2) */
  shapeD[1][1] = -(1 + x[0]) * .25;

  /** Derivative w.r.t r for shape function 3 (node 3) */
  shapeD[2][0] = +(1 + x[1]) * .25;

  /** Derivative w.r.t s for shape function 3 (node 3) */
  shapeD[2][1] = +(1 + x[0]) * .25;

  /** Derivative w.r.t r for shape function 4 (node 4) */
  shapeD[3][0] = -(1 + x[1]) * .25;

  /** Derivative w.r.t s for shape function 4 (node 4) */
  shapeD[3][1] = +(1 - x[0]) * .25;

  return shapeD;
}



/**
 * Returns computes the determinant of the Jacobian Matrix
 * at a given point (r,s) with respect to the local
 * coordinate system.
 */
QuadC02D::Float
QuadC02D::JacobianMatrixDeterminant(const vnl_matrix<Float>& J) const
{
  /** Computes the determinant of the Jacobian matrix */
  return (J[0][0] * J[1][1]) - (J[1][0] * J[0][1]);
}



/**
 * Returns the inverse transformation matrix from the master element to a
 * quadrilateral element.
 */
vnl_matrix<QuadC02D::Float>
QuadC02D::ComputeJacobianInverse(const vnl_matrix<Float>& J, Float detJ) const
{
  vnl_matrix<Float> I(2,2);

  /** Computes the inverse of the shape functions derivatives */
  I[0][0] =  J[1][1] / detJ;
  I[0][1] = -J[0][1] / detJ;
  I[1][0] = -J[1][0] / detJ;
  I[1][1] =  J[0][0] / detJ;

  return I;
}



/**
 * Return a matrix with the cartesian derivatives of the shape functions.
 */
vnl_matrix<QuadC02D::Float>
QuadC02D::ComputeShapeFunctionCartDerivatives(const vnl_matrix<Float>& I,
        const vnl_matrix<Float>& shapeD) const
{
  vnl_matrix<Float> shapeINVD(4,2);

  for (int i=0; i<4; i++) {
    shapeINVD[i][0] = (shapeD[i][0] * I[0][0]) + (shapeD[i][1] * I[1][0]);
    shapeINVD[i][1] = (shapeD[i][1] * I[0][1]) + (shapeD[i][1] * I[1][1]);
  }

  return shapeINVD;
}



/**
 * Return the strain matrix.
 */
vnl_matrix<QuadC02D::Float>
QuadC02D::ComputeBMatrix(const vnl_matrix<Float>& shapeINVD) const
{
  vnl_matrix<Float> B(3,8);
  int p;

  /** Computes the inverse shape function derivatives */
  for (int i=0; i<4; i++) {
    /** Computes B index */
    p = i << 1;

    /** Compute B elements */
    B[0][p]   = shapeINVD[i][0];
    B[0][p+1] = 0;
    B[1][p]   = 0;
    B[1][p+1] = shapeINVD[i][1];
    B[2][p]   = shapeINVD[i][1];
    B[2][p+1] = shapeINVD[i][0];
  }

  return B;
}




/**
 * Return the result of multiplying the elastic constant
 * matrix by the strain matrix.
 */
vnl_matrix<QuadC02D::Float>
QuadC02D::ComputeDBMatrix(const vnl_matrix<Float>& D, const vnl_matrix<Float>& B) const
{
  vnl_matrix<Float> DB(3,8);

  for (int i=0; i<3; i++) {
    for (int j=0; j<8; j++) {
      DB[i][j] = 0;
      for (int k=0; k<3; k++) {
        DB[i][j] += D[i][k] * B[k][j];
      }
    }
  }

  return DB;
}




/**
 * Gets the indices of the nodes defining an edge
 */
void
QuadC02D::GetNode(int id, int& n1, int& n2) const
{
  switch (id) {
    case 0 :
      n1 = 0;
      n2 = 1;
      break;
    case 1 :
      n1 = 1;
      n2 = 2;
      break;
    case 2 :
      n1 = 2;
      n2 = 3;
      break;
    case 3 :
      n1 = 3;
      n2 = 1;
  }
}



/**
 * Gets the coordinates of a given node
 */
void
QuadC02D::GetNodeCoordinates(int n, Float& x, Float& y) const
{
  switch (n) {
    case 0 :
      x = m_node[0]->X;
      y = m_node[0]->Y;
      break;
    case 1 :
      x = m_node[1]->X;
      y = m_node[1]->Y;
      break;
    case 2 :
      x = m_node[2]->X;
      y = m_node[2]->Y;
      break;
    case 3 :
      x = m_node[3]->X;
      y = m_node[3]->Y;
  }
}



/**
 * Read the element from input stream
 */
void QuadC02D::Read( std::istream& f, void* info )
{
  int n;
  /**
   * Convert the info pointer to a usable objects
   */
  Node::ArrayType::Pointer nodes=static_cast<ReadInfoType*>(info)->m_node;
  Material::ArrayType::Pointer mats=static_cast<ReadInfoType*>(info)->m_mat;


  /** first call the parent's read function */
  Superclass::Read(f,info);

  try
  {
    /**
     * Read and set the material pointer
     */
    SkipWhiteSpace(f); f>>n; if(!f) goto out;
    m_mat=dynamic_cast<const MaterialStandard*>( &*mats->Find(n));

    /**
     * Read and set each of the four expected GNN
     */
    SkipWhiteSpace(f); f>>n; if(!f) goto out;
    m_node[0]=dynamic_cast<const NodeXY*>( &*nodes->Find(n));

    SkipWhiteSpace(f); f>>n; if(!f) goto out;
    m_node[1]=dynamic_cast<const NodeXY*>( &*nodes->Find(n));

    SkipWhiteSpace(f); f>>n; if(!f) goto out;
    m_node[2]=dynamic_cast<const NodeXY*>( &*nodes->Find(n));

    SkipWhiteSpace(f); f>>n; if(!f) goto out;
    m_node[3]=dynamic_cast<const NodeXY*>( &*nodes->Find(n));
  }
  catch ( FEMExceptionObjectNotFound e )
  {
    throw FEMExceptionObjectNotFound(__FILE__,__LINE__,"QuadC02D::Read()",e.m_baseClassName,e.m_GN);
  }


out:

  if( !f )
  { 
    throw FEMExceptionIO(__FILE__,__LINE__,"QuadC02D::Read()","Error reading FEM element!");
  }

}




/**
 * Write the element to the output stream.
 */
void QuadC02D::Write( std::ostream& f, int ofid ) const {

  /** If not set already, se set the ofid */
  if (ofid<0) ofid=OFID;

  /** First call the parent's write function */
  Superclass::Write(f,ofid);

  /**
   * then the actual data (node, and material numbers)
   * we add some comments in the output file
   */
  f<<"\t"<<m_mat->GN<<"\t% MaterialStandard ID\n";
  f<<"\t"<<m_node[0]->GN<<"\t% NodeXY 1 ID\n";
  f<<"\t"<<m_node[1]->GN<<"\t% NodeXY 2 ID\n";
  f<<"\t"<<m_node[2]->GN<<"\t% NodeXY 3 ID\n";
  f<<"\t"<<m_node[3]->GN<<"\t% NodeXY 4 ID\n";

  /** check for errors */
  if (!f)
  { 
    throw FEMExceptionIO(__FILE__,__LINE__,"QuadC02D::Write()","Error writing FEM element!");
  }

}

FEM_CLASS_REGISTER(QuadC02D)




}} // end namespace itk::fem
