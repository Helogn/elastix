/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/
#ifndef __elxActiveRegistrationModelShapeMetric_hxx__
#define __elxActiveRegistrationModelShapeMetric_hxx__

#include <typeinfo>
#include <iterator>

namespace elastix
{

/**
 * ******************* Initialize ***********************
 */

template< class TElastix >
void
ActiveRegistrationModelShapeMetric< TElastix >
::Initialize( void )
{
  itk::TimeProbe timer;
  timer.Start();
  this->Superclass1::Initialize();
  timer.Stop();
  elxout << "Initialization of ActiveRegistrationModel metric took: "
         << static_cast< long >( timer.GetMean() * 1000 ) << " ms." << std::endl;

} // end Initialize()


/**
 * ***************** BeforeAllBase ***********************
 */

template< class TElastix >
int
ActiveRegistrationModelShapeMetric< TElastix >
::BeforeAllBase( void )
{

  this->Superclass2::BeforeAllBase();

  std::string componentLabel( this->GetComponentLabel() );
  std::string metricNumber = componentLabel.substr( 6, 2 ); // strip "Metric" keep number
  this->SetMetricNumber( std::stoul( metricNumber ) );
  
  // Paths to shape models for loading
  this->m_LoadShapeModelFileNames = ReadPath( std::string("LoadShapeModel") );

  // Paths to shape models for loading
  this->m_SaveShapeModelFileNames = ReadPath( std::string("SaveShapeModel") );
  
  // Paths to directories with shapes for model building
  this->m_ShapeDirectories = ReadPath( std::string("BuildShapeModel") );

  if( this->m_SaveShapeModelFileNames.size() > 0 )
  {
    if( this->m_SaveShapeModelFileNames.size() != this->m_ShapeDirectories.size() )
    {
      itkExceptionMacro( "The number of destinations for saving shape models must match the number of directories." )
    }
  }
  
  if( this->m_ShapeDirectories.size() > 0 )
  {
    // Reference shapes for model building
    this->m_ReferenceFilenames = ReadPath("ReferenceShape");

    if (this->m_ReferenceFilenames.size() != this->m_ShapeDirectories.size())
    {
      itkExceptionMacro(<< "The number of reference shapes does not match the number of directories given.");
    }
  }
  
  // At least one model must be specified
  if( 0 == ( this->m_LoadShapeModelFileNames.size() + this->m_ShapeDirectories.size() ) )
  {
    itkExceptionMacro( << "No statistical shape model specified for " << this->GetComponentLabel() << "." << std::endl
                       << "  Specify previously built models with (LoadShapeModel" << this->GetMetricNumber()
                       << " \"path/to/hdf5/file1\" \"path/to/hdf5/file2\" ) or " << std::endl 
                       << "  specify directories with shapes using (BuildShapeModel" << this->GetMetricNumber()
                       << " \"path/to/directory1\" \"path/to/directory2\") and " << std::endl
                       << "  corresponding reference shapes using \"(ReferenceShape" << this->GetMetricNumber()
                       << " \"path/to/reference1\" \"path/to/reference2\")." << std::endl
    );
  }
  
  return 0;

} // end BeforeAllBase()



/**
 * ***************** BeforeRegistration ***********************
 */

template< class TElastix >
void
ActiveRegistrationModelShapeMetric< TElastix >
::BeforeRegistration( void )
{
  StatisticalModelVectorContainerPointer statisticalModelMeanVectorContainer = StatisticalModelVectorContainerType::New();
  statisticalModelMeanVectorContainer->Reserve( this->m_LoadShapeModelFileNames.size() + this->m_ShapeDirectories.size() );

  StatisticalModelMatrixContainerPointer statisticalModelOrthonormalPCABasisMatrixContainer = StatisticalModelMatrixContainerType::New();
  statisticalModelOrthonormalPCABasisMatrixContainer->Reserve( this->m_LoadShapeModelFileNames.size() + this->m_ShapeDirectories.size() );

  StatisticalModelVectorContainerPointer statisticalModelVarianceVectorContainer = StatisticalModelVectorContainerType::New();
  statisticalModelVarianceVectorContainer->Reserve( this->m_LoadShapeModelFileNames.size() + this->m_ShapeDirectories.size() );

  StatisticalModelScalarContainerPointer statisticalModelNoiseVarianceContainer = StatisticalModelScalarContainerType::New();
  statisticalModelNoiseVarianceContainer->Reserve( this->m_LoadShapeModelFileNames.size() + this->m_ShapeDirectories.size() );

  StatisticalModelContainerPointer statisticalModelContainer = StatisticalModelContainerType::New();
  statisticalModelContainer->Reserve( this->m_LoadShapeModelFileNames.size() + this->m_ShapeDirectories.size() );

  // Load models
  if( this->m_LoadShapeModelFileNames.size() > 0 )
  {
    elxout << std::endl << "Loading models for " << this->GetComponentLabel() << ":" << this->elxGetClassName() << " ... " << std::endl;
    
    for( StatisticalModelIdType statisticalModelId = 0; statisticalModelId < this->m_LoadShapeModelFileNames.size(); ++statisticalModelId )
    {
      // Load model
      StatisticalModelPointer statisticalModel;
      try
      {
        StatisticalModelRepresenterPointer representer = StatisticalModelRepresenterType::New();
        statisticalModel = itk::StatismoIO< StatisticalModelMeshType >::LoadStatisticalModel( representer, this->m_LoadShapeModelFileNames[ statisticalModelId ] );
        statisticalModelMeanVectorContainer->SetElement( statisticalModelId, statisticalModel->GetMeanVector() );
        statisticalModelOrthonormalPCABasisMatrixContainer->SetElement( statisticalModelId, statisticalModel->GetOrthonormalPCABasisMatrix() );
        statisticalModelVarianceVectorContainer->SetElement( statisticalModelId, statisticalModel->GetPCAVarianceVector() );
        statisticalModelNoiseVarianceContainer->SetElement( statisticalModelId, statisticalModel->GetNoiseVariance() );
        statisticalModelContainer->SetElement( statisticalModelId, statisticalModel );
      }
      catch( statismo::StatisticalModelException &e )
      {
        itkExceptionMacro( "Error loading statistical shape model: " << e.what() );
      }
      
      elxout << "  Loaded model " << this->m_LoadShapeModelFileNames[ statisticalModelId ].c_str() << "." << std::endl;
      elxout << "  Number of principal components: " << statisticalModel->GetNumberOfPrincipalComponents() << "." << std::endl;
      elxout << "  Noise variance: " << statisticalModel->GetNoiseVariance() << "." << std::endl;
    }
  }
  
  // Build models
  if( this->m_ShapeDirectories.size() > 0 )
  {
    elxout << std::endl << "Building models for " << this->GetComponentLabel() << ":" << this->elxGetClassName() << " ... " << std::endl;

    // Noise parameter for probabilistic pca model
    StatisticalModelVectorType noiseVariance = this->ReadNoiseVariance();

    // Number of principal components to keep by variance
    StatisticalModelVectorType totalVariance = this->ReadTotalVariance();
    
    // Loop over all data directories
    for( StatisticalModelIdType statisticalModelId = 0; statisticalModelId < this->m_ShapeDirectories.size(); ++statisticalModelId )
    { 
      // Load data
      StatisticalModelDataManagerPointer dataManager;
      try 
      {
        dataManager = this->ReadMeshesFromDirectory(this->m_ShapeDirectories[ statisticalModelId ],
                                                    this->m_ReferenceFilenames[ statisticalModelId ]);
      }
      catch( statismo::StatisticalModelException &e )
      {
        itkExceptionMacro( "Error loading samples in " << this->m_ShapeDirectories[ statisticalModelId ] <<": " << e.what() );
      }
      
      // Build model
      elxout << "  Building statistical shape model for metric " << this->GetMetricNumber() << "... ";
      StatisticalModelPointer statisticalModel;
      try
      {
        StatisticalModelBuilderPointer pcaModelBuilder = StatisticalModelBuilderType::New();
        statisticalModel = pcaModelBuilder->BuildNewModel( dataManager->GetData(), noiseVariance[ statisticalModelId ] );
        elxout << "  Done."
               << "  Number of modes: " << statisticalModel->GetNumberOfPrincipalComponents() << "." << std::endl
               << "  Noise variance: " << statisticalModel->GetNoiseVariance()
               << "." << std::endl;
        
        // Pick out first principal components
        if( totalVariance[ statisticalModelId ] < 1.0 )
        {
          elxout << "  Reducing model to " << totalVariance[ statisticalModelId ] * 100.0 << "% variance ... ";
          StatisticalModelReducedVarianceBuilderPointer reducedVarianceModelBuilder = StatisticalModelReducedVarianceBuilderType::New();
          statisticalModel = reducedVarianceModelBuilder->BuildNewModelWithVariance( statisticalModel, totalVariance[ statisticalModelId ] );
          elxout << " Done." << std::endl
                 << "  Number of modes retained: " << statisticalModel->GetNumberOfPrincipalComponents() << "." << std::endl;
        }
      }
      catch( statismo::StatisticalModelException& e )
      {
        itkExceptionMacro( << "Error building statistical shape model: " << e.what() );
      }

      if( this->m_SaveShapeModelFileNames.size() > 0 )
      {
        elxout << "  Saving shape model " << statisticalModelId << " to " << this->m_SaveShapeModelFileNames[ statisticalModelId ] << ". " << std::endl;
        try
        {
          itk::StatismoIO< StatisticalModelMeshType >::SaveStatisticalModel(statisticalModel, this->m_SaveShapeModelFileNames[ statisticalModelId ]);
        }
        catch( statismo::StatisticalModelException& e )
        {
          itkExceptionMacro( "Could not save shape model to " << this->m_SaveShapeModelFileNames[ statisticalModelId ] << ".");
        }
      }

      statisticalModelMeanVectorContainer->SetElement( statisticalModelId, statisticalModel->GetMeanVector());
      statisticalModelOrthonormalPCABasisMatrixContainer->SetElement( statisticalModelId, statisticalModel->GetOrthonormalPCABasisMatrix());
      statisticalModelVarianceVectorContainer->SetElement( statisticalModelId, statisticalModel->GetPCAVarianceVector() );
      statisticalModelNoiseVarianceContainer->SetElement( statisticalModelId, noiseVariance[ statisticalModelId ]);
      statisticalModelContainer->SetElement( statisticalModelId, statisticalModel );
    }
  }
  
  this->SetMeanVectorContainer( statisticalModelMeanVectorContainer );
  this->SetBasisMatrixContainer( statisticalModelOrthonormalPCABasisMatrixContainer );
  this->SetVarianceContainer( statisticalModelVarianceVectorContainer );
  this->SetNoiseVarianceContainer( statisticalModelNoiseVarianceContainer );

  this->SetStatisticalModelContainer( statisticalModelContainer );

  // SingleValuedPointSetToPointSetMetric (from which this class is derived) needs a fixed and moving point set
  typename FixedPointSetType::Pointer fixedDummyPointSet = FixedPointSetType::New();
  typename MovingPointSetType::Pointer movingDummyPointSet = MovingPointSetType::New();
  this->SetFixedPointSet( fixedDummyPointSet );  // FB: TODO solve hack
  this->SetMovingPointSet( movingDummyPointSet ); // FB: TODO solve hack

  std::cout << std::endl;
} // end BeforeRegistration()



/**
 * ***************** loadShapesFromDirectory ***********************
 */

template< class TElastix >
typename ActiveRegistrationModelShapeMetric< TElastix >::StatisticalModelDataManagerPointer
ActiveRegistrationModelShapeMetric< TElastix >
::ReadMeshesFromDirectory(
        std::string shapeDataDirectory,
        std::string referenceFilename)
{
  
  itk::Directory::Pointer directory = itk::Directory::New();
  if( !directory->Load( shapeDataDirectory.c_str() ) )
  {
    itkExceptionMacro( "No files found in " << shapeDataDirectory << ".");
  }
  
  // Read reference shape
  StatisticalModelMeshPointer reference = StatisticalModelMeshType::New();
  if( this->ReadMesh( referenceFilename, reference ) == 0 )
  {
    itkExceptionMacro( "Failed to read reference file " << referenceFilename << ".");
  }

  StatisticalModelRepresenterPointer representer = StatisticalModelRepresenterType::New();
  representer->SetReference( reference );

  StatisticalModelDataManagerPointer dataManager = StatisticalModelDataManagerType::New();
  dataManager->SetRepresenter( representer.GetPointer() );
  
  for( int i = 0; i < directory->GetNumberOfFiles(); ++i )
  {
    const char * filename = directory->GetFile( i );
    if( std::strcmp( filename, referenceFilename.c_str() ) == 0 || std::strcmp( filename, "." ) == 0 || std::strcmp( filename, ".." ) == 0 )
    {
      continue;
    }

    std::string fullpath = shapeDataDirectory + "/" + filename;
    StatisticalModelMeshPointer mesh = StatisticalModelMeshType::New();

    unsigned long numberOfMeshPoints = this->ReadMesh( fullpath.c_str(), mesh );
    if( numberOfMeshPoints > 0 )
    {
      dataManager->AddDataset( mesh, fullpath.c_str() );
    }
  }
  
  return dataManager;
}



/**
 * ************** ReadShape *********************
 */

template< class TElastix >
unsigned long
ActiveRegistrationModelShapeMetric< TElastix >
::ReadMesh(
  const std::string& meshFilename,
  StatisticalModelMeshPointer& mesh )
{
  // Read the input mesh. */
  MeshReaderPointer meshReader = MeshReaderType::New();
  meshReader->SetFileName( meshFilename.c_str() );

  elxout << "  Reading input mesh file: " << meshFilename << " ... ";
  try
  {
    meshReader->UpdateLargestPossibleRegion();
  }
  catch( itk::ExceptionObject & err )
  {
    elxout << "skipping " << meshFilename << " (not a valid mesh file or file does not exist)." << std::endl;
    return 0;
  }

  // Some user-feedback. 
  mesh = meshReader->GetOutput();
  unsigned long numberOfPoints = mesh->GetNumberOfPoints();
  if( numberOfPoints == 0 )
  {
    elxout << "read " << numberOfPoints << " points." << std::endl;
  }
  else
  {
    elxout << "skipping " << meshFilename << " (no points in mesh file)." << std::endl;
  }

  return numberOfPoints;
} // end ReadMesh()



/**
 * ******************* WriteMesh ********************
 */

template< class TElastix >
void
ActiveRegistrationModelShapeMetric< TElastix >
::WriteMesh( const char * filename, StatisticalModelMeshType mesh )
{
  // Create writer.
  MeshFileWriterPointer meshWriter = MeshFileWriterType::New();

  meshWriter->SetInput( mesh );
  meshWriter->SetFileName( filename );

  try
  {
    meshWriter->Update();
  }
  catch( itk::ExceptionObject & excp )
  {
    // Add information to the exception.
    excp.SetLocation( "ActiveRegistrationModel - WriteMesh()" );
    std::string err_str = excp.GetDescription();
    err_str += "\nError occurred while writing mesh.\n";
    excp.SetDescription( err_str );

    // Pass the exception to an higher level.
    throw excp;
  }
} // end WriteMesh()



/**
 * ***************** ReadPath ***********************
 */

template< class TElastix >
typename ActiveRegistrationModelShapeMetric< TElastix>::StatisticalModelPathVectorType
ActiveRegistrationModelShapeMetric< TElastix >
::ReadPath( std::string path )
{
  std::ostringstream key;
  key << path << this->GetMetricNumber();

  StatisticalModelPathVectorType pathVector;
  for( unsigned int i = 0; i < this->GetConfiguration()->CountNumberOfParameterEntries( key.str() ); ++i )
  {
    std::string value = "";
    this->m_Configuration->ReadParameter( value, key.str(), i );
    pathVector.push_back( value );
  }
  
  return pathVector;
}



/**
 * ***************** ReadNoiseVariance ***********************
 */

template< class TElastix >
typename ActiveRegistrationModelShapeMetric< TElastix >::StatisticalModelVectorType
ActiveRegistrationModelShapeMetric< TElastix >
::ReadNoiseVariance()
{
  std::ostringstream key( "NoiseVariance", std::ios_base::ate );
  key << this->GetMetricNumber();

  StatisticalModelVectorType noiseVarianceVector = StatisticalModelVectorType( this->m_ShapeDirectories.size(), 0.0 );
  unsigned int n = this->GetConfiguration()->CountNumberOfParameterEntries( key.str() );
  
  if( n == 0 )
  {
    elxout << "WARNING: NoiseVariance not specified for " << this->GetComponentLabel() << ":" << this->elxGetClassName() << "." << std::endl
           << "  A default value of " << noiseVarianceVector[ 0 ] << " will be used (non-probabilistic PCA) for metric " << this->GetMetricNumber() << "." << std::endl;
    
    return noiseVarianceVector;
  }
  
  for(unsigned int i = 0; i < this->GetConfiguration()->CountNumberOfParameterEntries( key.str() ); ++i)
  {
    std::string value = "";
    this->m_Configuration->ReadParameter( value, key.str(), i );
    
    char *e;
    errno = 0;
    double noiseVariance = std::strtod( value.c_str(), &e );
    
    if ( *e != '\0' || // error, we didn't consume the entire string
         errno != 0 )  // error, overflow or underflow
    {
      itkExceptionMacro( << "Invalid number format for NoiseVariance entry " << i << "." );
    }
    
    if( noiseVariance < 0 )
    {
      itkExceptionMacro( << "NoiseVariance entry number " << i << " is negative (" << noiseVariance << "). Variance must be positive by definition. Please correct your parameter file." );
    }
    
    noiseVarianceVector[ i ] = noiseVariance;
  }
  
  if( n == 1 && noiseVarianceVector.size() > 1 )
  {
    // Fill the rest of the elements
    noiseVarianceVector.fill( noiseVarianceVector[ 0 ] );
  }
  
  return noiseVarianceVector;
}



/**
 * ***************** ReadTotalVariance ***********************
 */

template< class TElastix >
typename ActiveRegistrationModelShapeMetric< TElastix >::StatisticalModelVectorType
ActiveRegistrationModelShapeMetric< TElastix >
::ReadTotalVariance()
{
  std::ostringstream key( "TotalVariance", std::ios_base::ate );
  key << this->GetMetricNumber();

  StatisticalModelVectorType totalVarianceVector = StatisticalModelVectorType( this->m_ShapeDirectories.size(), 1.0 );
  unsigned int n = this->GetConfiguration()->CountNumberOfParameterEntries( key.str() );
  
  if( n == 0 )
  {
    elxout << "WARNING: TotalVariance not specified for " << this->GetComponentLabel() << ":" << this->elxGetClassName() << "." << std::endl
           << "  A default value of 1.0 will be used (all principal componontents) for metric " << this->GetMetricNumber() << "." << std::endl;
    
    return totalVarianceVector;
  }
  
  for(unsigned int i = 0; i < this->GetConfiguration()->CountNumberOfParameterEntries( key.str() ); ++i)
  {
    std::string value = "";
    this->m_Configuration->ReadParameter( value, key.str(), i );
    
    char *e;
    errno = 0;
    double totalVariance = std::strtod( value.c_str(), &e );
    
    if ( *e != '\0' || // error, we didn't consume the entire string
         errno != 0 )  // error, overflow or underflow
    {
      itkExceptionMacro( << "Invalid number format for NoiseVariance entry " << i << "." );
    }
    
    if( totalVariance < 0.0 || totalVariance > 1.0 )
    {
      itkExceptionMacro( << "TotalVariance entries must lie in [0.0; 1.0] but entry number " << i << " is " << totalVariance << ". Please correct your parameter file." );
    }
    
    totalVarianceVector[ i ] = totalVariance;
  }
  
  if( n == 1 && totalVarianceVector.size() > 1 )
  {
    // Need to fill the rest of the elements
    totalVarianceVector.fill( totalVarianceVector[ 0 ] );
  }
  
  return totalVarianceVector;
}



/**
 * ***************** AfterRegistration ***********************
 */

template< class TElastix >
void
ActiveRegistrationModelShapeMetric< TElastix >
::AfterRegistration( void )
{
  const unsigned int level = this->m_Registration->GetAsITKBaseType()->GetCurrentLevel();

  /** Decide whether or not to write the mean images */
  bool writeShapeModelMeanShape = false;
  this->m_Configuration->ReadParameter( writeShapeModelMeanShape,
                                        "WriteShapeModelMeanShapeAfterRegistration", "", level, 0, false );

  if( writeShapeModelMeanShape )
  {
//    for( unsigned int i = 0; i < this->GetStatisticalModelContainer()->Size(); i++ )
//    {
//
//      std::ostringstream makeFileName( "" );
//      makeFileName
//      << this->m_Configuration->GetCommandLineArgument( "-out" )
//      << "ShapeModel" << i
//      << "Metric" << this->GetMetricNumber()
//      << "MeanShape.vtk";
//
//      elxout << "  Writing statistical model mean shape " << i << " for " << this->GetComponentLabel() << " to " << makeFileName.str() << std::endl;
//
//      MeshFileWriterPointer meshWriter = MeshFileWriterType::New();
//      meshWriter->SetInput( this->GetStatisticalModelContainer()->GetElement( i )->DrawMean() );
//      meshWriter->SetFileName( makeFileName.str() );
//      meshWriter->Update();
//    }
  }

  /** Decide whether or not to write final model image */
  bool writeShapeModelFinalShape = false;
  this->m_Configuration->ReadParameter( writeShapeModelFinalShape,
                                        "WriteShapeModelFinalShapeAfterRegistration", "", level, 0, false );

  if( writeShapeModelFinalShape )
  {
// TODO: For now we just warp with transformix after registration
//    for( unsigned int i = 0; i < this->GetStatisticalModelContainer()->Size(); i++ )
//    {
//      std::string meanImageFormat = "nii.gz";
//      this->m_Configuration->ReadParameter( meanImageFormat, "ResultImageFormat", 0, false );
//
//      std::ostringstream makeFileName( "" );
//      makeFileName
//      << this->m_Configuration->GetCommandLineArgument( "-out" )
//      << "ImageIntensityModel" << i
//      << "Metric" << this->GetMetricNumber()
//      << "FinalImage." << meanImageFormat;
//
//      elxout << "  Writing statistical model final image " << i << " for " << this->GetComponentLabel() << " to " << makeFileName.str() << std::endl;
//
//      // This happens after Applying final transform so no need to update resampler
//
//      StatisticalModelVectorType modelCoefficients = this->GetStatisticalModelContainer()->GetElement( i )->ComputeCoefficientsForDataset( movingMesh );
//
//      MeshFileWriterPointer meshWriter = MeshFileWriterType::New();
//      meshWriter->SetInput( this->GetStatisticalModelContainer()->GetElement( i )->DrawMean() );
//      meshWriter->SetFileName( makeFileName.str() );
//      meshWriter->Update();
//    }
  }

  bool writeShapeModelPrincipalComponents = false;
  this->m_Configuration->ReadParameter( writeShapeModelPrincipalComponents,
                                        "WriteShapeModelPrincipalComponentsAfterRegistration", "", level, 0, false );

  if( writeShapeModelPrincipalComponents )
  {
//    for( unsigned int i = 0; i < this->GetStatisticalModelContainer()->Size(); i++ )
//    {
//      // TODO: Do loop and lets see model
//      StatisticalModelVectorType variance = this->GetStatisticalModelContainer()->GetElement( i )->GetPCAVarianceVector();
//      MeshFileWriterPointer imageWriter = MeshFileWriterType::New();
//
//      // 1st principal component
//      StatisticalModelVectorType pc0plus3std = StatisticalModelVectorType( this->GetStatisticalModelContainer()->GetElement( i )->GetNumberOfPrincipalComponents(), 0.0 );
//      pc0plus3std[ 0 ] = 3.0;
//
//      std::ostringstream makeFileNamePC0P3STD( "" );
//      makeFileNamePC0P3STD
//      << this->m_Configuration->GetCommandLineArgument( "-out" )
//      << "ShapeModel" << i
//      << "Metric" << this->GetMetricNumber()
//      << "PC0plus3std.vtk";
//
//      elxout << "  Writing statistical model principal component 0 plus 3 standard deviations for model " << i << " for " << this->GetComponentLabel() << " to " << makeFileNamePC0P3STD.str() << std::endl;
//
//      MeshFileWriterPointer meshWriter = MeshFileWriterType::New();
//      meshWriter->SetInput( this->GetStatisticalModelContainer()->GetElement( i )->DrawMean() );
//      meshWriter->SetFileName( makeFileNamePC0P3STD.str() );
//      meshWriter->Update();
//
//      StatisticalModelVectorType pc0minus3std = StatisticalModelVectorType( this->GetStatisticalModelContainer()->GetElement( i )->GetNumberOfPrincipalComponents(), 0.0 );
//      pc0minus3std[ 0 ] = -3.0;
//
//      std::ostringstream makeFileNamePC0M3STD( "" );
//      makeFileNamePC0M3STD
//      << this->m_Configuration->GetCommandLineArgument( "-out" )
//      << "ShapeModel" << i
//      << "Metric" << this->GetMetricNumber()
//      << "PC0minus3std.vtk";
//
//      elxout << "  Writing statistical model principal component 0 minus 3 standard deviations for model " << i << " for " << this->GetComponentLabel() << " to " << makeFileNamePC0M3STD.str() << std::endl;
//      meshWriter->SetInput( this->GetStatisticalModelContainer()->GetElement( i )->DrawMean() );
//      meshWriter->SetFileName( makeFileNamePC0M3STD.str() );
//      meshWriter->Update();
//
//      // 2nd principal component
//      StatisticalModelVectorType pc1plus3std = StatisticalModelVectorType( this->GetStatisticalModelContainer()->GetElement( i )->GetNumberOfPrincipalComponents(), 0.0 );
//      pc1plus3std[ 1 ] = 3.0;
//
//      std::ostringstream makeFileNamePC1P3STD( "" );
//      makeFileNamePC1P3STD
//      << this->m_Configuration->GetCommandLineArgument( "-out" )
//      << "ShapeModel" << i
//      << "Metric" << this->GetMetricNumber()
//      << "PC1plus3std.vtk";
//
//      elxout << "  Writing statistical model principal component 1 plus 3 standard deviations for model " << i << " for " << this->GetComponentLabel() << " to " << makeFileNamePC1P3STD.str() << std::endl;
//      meshWriter->SetInput( this->GetStatisticalModelContainer()->GetElement( i )->DrawMean() );
//      meshWriter->SetFileName( makeFileNamePC1P3STD.str() );
//      meshWriter->Update();
//
//      //
//      StatisticalModelVectorType pc1minus3std = StatisticalModelVectorType( this->GetStatisticalModelContainer()->GetElement( i )->GetNumberOfPrincipalComponents(), 0.0 );
//      pc1minus3std[ 1 ] = -3.0;
//
//      std::ostringstream makeFileNamePC1M3STD( "" );
//      makeFileNamePC1M3STD
//      << this->m_Configuration->GetCommandLineArgument( "-out" )
//      << "ShapeModel" << i
//      << "Metric" << this->GetMetricNumber()
//      << "PC1minus3std.vtk";
//
//      elxout << "  Writing statistical model principal component 1 minus 3 standard deviations for model " << i << " for " << this->GetComponentLabel() << " to " << makeFileNamePC1M3STD.str() << std::endl;
//      meshWriter->SetInput( this->GetStatisticalModelContainer()->GetElement( i )->DrawMean() );
//      meshWriter->SetFileName( makeFileNamePC1M3STD.str() );
//      meshWriter->Update();
//
//      // 3rd principal component
//      StatisticalModelVectorType pc2plus3std = StatisticalModelVectorType( this->GetStatisticalModelContainer()->GetElement( i )->GetNumberOfPrincipalComponents(), 0.0 );
//      pc2plus3std[ 2 ] = 3.0;
//
//      std::ostringstream makeFileNamePC2P3STD( "" );
//      makeFileNamePC2P3STD
//      << this->m_Configuration->GetCommandLineArgument( "-out" )
//      << "ShapeModel" << i
//      << "Metric" << this->GetMetricNumber()
//      << "PC2plus3std.vtk";
//
//      elxout << "  Writing statistical model principal component 2 plus 3 standard deviations for model " << i << " for " << this->GetComponentLabel() << " to " << makeFileNamePC2P3STD.str() << std::endl;
//      meshWriter->SetInput( this->GetStatisticalModelContainer()->GetElement( i )->DrawMean() );
//      meshWriter->SetFileName( makeFileNamePC2P3STD.str() );
//      meshWriter->Update();
//
//      //
//      StatisticalModelVectorType pc2minus3std = StatisticalModelVectorType( this->GetStatisticalModelContainer()->GetElement( i )->GetNumberOfPrincipalComponents(), 0.0 );
//      pc2minus3std[ 2 ] = -3.0;
//
//      std::ostringstream makeFileNamePC2M3STD( "" );
//      makeFileNamePC2M3STD
//      << this->m_Configuration->GetCommandLineArgument( "-out" )
//      << "ShapeModel" << i
//      << "Metric" << this->GetMetricNumber()
//      << "PC2minus3std.vtk";
//
//      elxout << "  Writing statistical model principal component 2 minus 3 standard deviations for model " << i << " for " << this->GetComponentLabel() << " to " << makeFileNamePC2M3STD.str() << std::endl;
//      meshWriter->SetInput( this->GetStatisticalModelContainer()->GetElement( i )->DrawMean() );
//      meshWriter->SetFileName( makeFileNamePC2M3STD.str() );
//      meshWriter->Update();
//    }
  }
} // end AfterRegistration()

} // end namespace elastix

#endif // end #ifndef __elxActiveRegistrationModelShapeMetric_hxx__

