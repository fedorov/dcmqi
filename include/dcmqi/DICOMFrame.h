//
// Created by Andrey Fedorov on 3/9/17.
//

#ifndef DCMQI_DICOMFRAME_H
#define DCMQI_DICOMFRAME_H

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dctk.h>
#include "ImageVolumeGeometry.h"
#include "Exceptions.h"

namespace dcmqi {

  // Describe input/output frames for the purposes of sorting and associating with the
  // slices of the ITK volume
  class DICOMFrame {
  public:
    // distinguish between the frames from the legacy data and enhanced multiframe objects
    enum {
      LegacyInstanceFrame = 0,
      EnhancedInstanceFrame
    };

    DICOMFrame(DcmDataset *dataset, int number=0) :
        frameNumber(number),
        frameDataset(dataset) {

      Uint32 numFrames;
      if(dataset->findAndGetUint32(DCM_NumberOfFrames, numFrames).good()){
        frameType = EnhancedInstanceFrame;
        if(!number){
          std::cerr << "ERROR: DICOMFrame object for an enhanced frame is initialized with frame 0!" << std::endl;
        }
      } else {
        frameType = LegacyInstanceFrame;
        initializeFrameGeometryFromLegacyInstance();
      }

      OFString seriesUIDOF, instanceUIDOF, classUIDOF;
      if(dataset->findAndGetOFString(DCM_SeriesInstanceUID, seriesUIDOF).good()){
        seriesUID = seriesUIDOF.c_str();
      }
      if(dataset->findAndGetOFString(DCM_SOPInstanceUID, instanceUIDOF).good()){
        instanceUID = instanceUIDOF.c_str();
      }
      if(dataset->findAndGetOFString(DCM_SOPClassUID, classUIDOF).good()){
        classUID = classUIDOF.c_str();
      }

    };

    int getFrameNumber() const {
      return frameNumber; // 0 for legacy datasets, 1 or above for enhanced objects
    };

    vnl_vector<double> getFrameIPP(){
      return frameIPP;
    };

    string getSeriesUID() const {
      return seriesUID;
    }

    string getInstanceUID() const{
      return instanceUID;
    }

    string getClassUID() const {
      return classUID;
    }

    DcmDataset* getDataset() const {
      return frameDataset;
    }

  private:

    int initializeFrameGeometryFromLegacyInstance();
    int initializeFrameGeometryFromEnhancedInstance();

    int frameType;
    DcmDataset *frameDataset;
    int frameNumber;
    vnl_vector<double> frameIPP;

    string seriesUID, instanceUID, classUID;

    ImageVolumeGeometry frameGeometry;

  };

  struct DICOMFrame_compare {
    bool operator() (const DICOMFrame& lhs, const DICOMFrame& rhs) const{
      std::stringstream s1,s2;
      s1 << lhs.getInstanceUID();
      s2 << rhs.getInstanceUID();
      return (s1.str() < s2.str()) && (lhs.getFrameNumber() < rhs.getFrameNumber());
    }
  };

}

#endif //DCMQI_DICOMFRAME_H
