/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

#ifndef __itkANNStandardTreeSearch_h
#define __itkANNStandardTreeSearch_h

#include "itkBinaryANNTreeSearchBase.h"


namespace itk
{
  
  /**
   * \class ANNStandardTreeSearch
   *
   * \brief 
   *
   * 
   * \ingroup Metrics?
   */
  
  template < class TListSample >
  class ANNStandardTreeSearch : public BinaryANNTreeSearchBase< TListSample >
  {
  public:
    
    /** Standard itk. */
    typedef ANNStandardTreeSearch                   Self;
    typedef BinaryANNTreeSearchBase< TListSample >  Superclass;
    typedef SmartPointer< Self >                    Pointer;
    typedef SmartPointer< const Self >              ConstPointer;
    
    /** New method for creating an object using a factory. */
    itkNewMacro( Self );

    /** ITK type info. */
    itkTypeMacro( ANNStandardTreeSearch, BinaryANNTreeSearchBase );

    /** Typedefs from Superclass. */
    typedef typename Superclass::ListSampleType         ListSampleType;
    typedef typename Superclass::BinaryTreeType         BinaryTreeType;
    typedef typename Superclass::MeasurementVectorType  MeasurementVectorType;
    typedef typename Superclass::IndexArrayType         IndexArrayType;
    typedef typename Superclass::DistanceArrayType      DistanceArrayType;
    
    typedef typename Superclass::ANNPointType           ANNPointType;         // double *
    typedef typename Superclass::ANNIndexType           ANNIndexType;         // int
    typedef typename Superclass::ANNIndexArrayType      ANNIndexArrayType;    // int *
    typedef typename Superclass::ANNDistanceType        ANNDistanceType;      // double
    typedef typename Superclass::ANNDistanceArrayType   ANNDistanceArrayType; // double *

    typedef typename Superclass::BinaryANNTreeType      BinaryANNTreeType;

    /** Set and get the error bound eps. */
    itkSetClampMacro( ErrorBound, double, 0.0, 1e14 );
    itkGetConstMacro( ErrorBound, double );

    /** Search the nearest neighbours of a query point qp. */
    virtual void Search( const MeasurementVectorType & qp, IndexArrayType & ind,
      DistanceArrayType & dists );

    /** Set the maximum number of points that are to be visited. */
    //void SetMaximumNumberOfPointsToVisit( unsigned int num )
    //{
    //  annMaxPtsVisit( static_cast<int>( num ) );
    //}

    /** Get the number of points that are visited in the last search. *
    unsigned int GetNumberOfVisitedPoints(void)
    {
      //return this->m_BinaryTree->GetANNTree()::ANNptsVisited;
      return ANNptsVisited;
    }*/


  protected:
    
    ANNStandardTreeSearch();
    virtual ~ANNStandardTreeSearch();

    /** Member variables. */
    double  m_ErrorBound;
    
  private:
    
    ANNStandardTreeSearch( const Self& ); // purposely not implemented
    void operator=( const Self& );        // purposely not implemented

  }; // end class ANNStandardTreeSearch
    
    
} // end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkANNStandardTreeSearch.txx"
#endif


#endif // end #ifndef __itkANNStandardTreeSearch_h

