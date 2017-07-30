//
// Created by Andrey Fedorov on 3/11/17.
//

#ifndef DCMQI_PARAMETRICMAPOBJECT_H
#define DCMQI_PARAMETRICMAPOBJECT_H


#include <dcmtk/dcmfg/fgframeanatomy.h>
#include <dcmtk/dcmfg/fgidentpixeltransform.h>
#include <dcmtk/dcmfg/fgparametricmapframetype.h>
#include <dcmtk/dcmfg/fgrealworldvaluemapping.h>
#include <dcmtk/dcmpmap/dpmparametricmapiod.h>
#include <dcmtk/dcmdata/dcrledrg.h>

#include <itkCastImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>

#include "MultiframeObject.h"

/*
 *
 */
class ParametricMapObject : public MultiframeObject {
public:

  ParametricMapObject(){
    parametricMap = NULL;
  }

  typedef IODFloatingPointImagePixelModule::value_type Float32PixelType;
  typedef itk::Image<Float32PixelType, 3> Float32ITKImageType;

  // metadata is mandatory, since not all of the attributes can be present
  //  in the derivation DcmDataset(s)
  int initializeFromITK(Float32ITKImageType::Pointer, const string&);
  // metadata is mandatory, optionally, derivation DcmDataset(s) can
  //  help
  int initializeFromITK(Float32ITKImageType::Pointer, const string&,
                        std::vector<DcmDataset*>);

  int updateMetaDataFromDICOM(std::vector<DcmDataset*>);

  int getDICOMRepresentation(DcmDataset& dcm){
    if(parametricMap)
      CHECK_COND(parametricMap->write(dcm));
  };

  int initializeFromDICOM(DcmDataset * sourceDataset);

  void initializeMetaDataFromDICOM();

  Float32ITKImageType::Pointer getITKRepresentation() const {
    return itkImage;
  }

protected:
  typedef itk::CastImageFilter<Float32ITKImageType,DummyImageType>
      Float32ToDummyCasterType;
  typedef itk::MinimumMaximumImageCalculator<Float32ITKImageType> MinMaxCalculatorType;

  int initializeEquipmentInfo();
  int initializeVolumeGeometry();
  int createDICOMParametricMap();
  int createITKParametricMap();
  int createITKImageFromFrames(FGInterface&, DPMParametricMapIOD::Frames<Float32PixelType>);
  int initializeCompositeContext();
  int initializeFrameAnatomyFG();
  int initializeRWVMFG();
  int initializeFrames(vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> >&);

  bool isDerivationFGRequired(vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> >& slice2frame);

  IODEnhGeneralEquipmentModule::EquipmentInfo enhancedEquipmentInfoModule;

  // Functional groups specific to PM:
  //  - Shared
  FGFrameAnatomy frameAnatomyFG;
  FGIdentityPixelValueTransformation identityPixelValueTransformationFG;
  FGParametricMapFrameType parametricMapFrameTypeFG;
  FGRealWorldValueMapping rwvmFG;

  // Data containers specific to this object
  Float32ITKImageType::Pointer itkImage;

private:
  DPMParametricMapIOD* parametricMap;
  CodeSequenceMacro *createCodeSequenceFromMetadata(const string &codeName) const;
};


#endif //DCMQI_PARAMETRICMAPOBJECT_H
