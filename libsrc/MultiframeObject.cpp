//
// Created by Andrey Fedorov on 3/11/17.
//

#include <dcmqi/Helper.h>
#include "dcmqi/QIICRConstants.h"
#include "dcmqi/MultiframeObject.h"

int MultiframeObject::initializeFromDICOM(std::vector<DcmDataset *> sourceDataset) {
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeMetaDataFromString(const std::string &metaDataStr) {
  std::istringstream metaDataStream(metaDataStr);
  metaDataStream >> metaDataJson;
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeEquipmentInfo() {
  if(sourceRepresentationType == ITK_REPR){
    equipmentInfoModule = IODEnhGeneralEquipmentModule::EquipmentInfo(QIICR_MANUFACTURER, QIICR_DEVICE_SERIAL_NUMBER,
                                                                      QIICR_MANUFACTURER_MODEL_NAME, QIICR_SOFTWARE_VERSIONS);
    /*
    equipmentInfoModule.m_Manufacturer = QIICR_MANUFACTURER;
    equipmentInfoModule.m_DeviceSerialNumber = QIICR_DEVICE_SERIAL_NUMBER;
    equipmentInfoModule.m_ManufacturerModelName = QIICR_MANUFACTURER_MODEL_NAME;
    equipmentInfoModule.m_SoftwareVersions = QIICR_SOFTWARE_VERSIONS;
    */

  } else { // DICOM_REPR
  }
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeContentIdentification() {

  if(sourceRepresentationType == ITK_REPR){
    CHECK_COND(contentIdentificationMacro.setContentCreatorName("dcmqi"));
    if(metaDataJson.isMember("ContentDescription")){
      CHECK_COND(contentIdentificationMacro.setContentDescription(metaDataJson["ContentDescription"].asCString()));
    } else {
      CHECK_COND(contentIdentificationMacro.setContentDescription("DCMQI"));
    }
    if(metaDataJson.isMember("ContentLabel")){
      CHECK_COND(contentIdentificationMacro.setContentLabel(metaDataJson["ContentLabel"].asCString()));
    } else {
      CHECK_COND(contentIdentificationMacro.setContentLabel("DCMQI"));
    }
    if(metaDataJson.isMember("InstanceNumber")){
      CHECK_COND(contentIdentificationMacro.setInstanceNumber(metaDataJson["InstanceNumber"].asCString()));
    } else {
      CHECK_COND(contentIdentificationMacro.setInstanceNumber("1"));
    }
    CHECK_COND(contentIdentificationMacro.check())
    return EXIT_SUCCESS;
  } else { // DICOM_REPR
  }
  return EXIT_SUCCESS;
}

int MultiframeObject::initializeVolumeGeometryFromITK(DummyImageType::Pointer image) {
  DummyImageType::SpacingType spacing;
  DummyImageType::PointType origin;
  DummyImageType::DirectionType directions;
  DummyImageType::SizeType extent;

  spacing = image->GetSpacing();
  directions = image->GetDirection();
  extent = image->GetLargestPossibleRegion().GetSize();
  origin = image->GetOrigin();

  volumeGeometry.setSpacing(spacing);
  volumeGeometry.setOrigin(origin);
  volumeGeometry.setExtent(extent);
  volumeGeometry.setDirections(directions);

  return EXIT_SUCCESS;
}

int MultiframeObject::initializeVolumeGeometryFromDICOM(FGInterface &fgInterface) {
  SpacingType spacing;
  PointType origin;
  DirectionType directions;
  SizeType extent;

  if (getImageDirections(fgInterface, directions)) {
    cerr << "Failed to get image directions" << endl;
    throw -1;
  }

  double computedSliceSpacing, computedVolumeExtent;
  vnl_vector<double> sliceDirection(3);
  sliceDirection[0] = directions[0][2];
  sliceDirection[1] = directions[1][2];
  sliceDirection[2] = directions[2][2];
  if (computeVolumeExtent(fgInterface, sliceDirection, origin, computedSliceSpacing, computedVolumeExtent)) {
    cerr << "Failed to compute origin and/or slice spacing!" << endl;
    throw -1;
  }

  if (getDeclaredImageSpacing(fgInterface, spacing)) {
    cerr << "Failed to get image spacing from DICOM!" << endl;
    throw -1;
  }

  const double tolerance = 1e-5;
  if(!spacing[2]){
    spacing[2] = computedSliceSpacing;
  } else if(fabs(spacing[2]-computedSliceSpacing)>tolerance){
    cerr << "WARNING: Declared slice spacing is significantly different from the one declared in DICOM!" <<
         " Declared = " << spacing[2] << " Computed = " << computedSliceSpacing << endl;
  }

  OFString str;
  if(dcmRepresentation->findAndGetOFString(DCM_Rows, str).good())
    extent[1] = atoi(str.c_str());
  if(dcmRepresentation->findAndGetOFString(DCM_Columns, str).good())
    extent[0] = atoi(str.c_str());

  cout << "computed extent: " << computedVolumeExtent << "/" << spacing[2] << endl;

  extent[2] = round(computedVolumeExtent/spacing[2] + 1);

  cout << "Extent:" << extent << endl;

  volumeGeometry.setSpacing(spacing);
  volumeGeometry.setOrigin(origin);
  volumeGeometry.setExtent(extent);
  volumeGeometry.setDirections(directions);

  return EXIT_SUCCESS;
}

// for now, we do not support initialization from JSON only,
//  and we don't have a way to validate metadata completeness - TODO!
bool MultiframeObject::metaDataIsComplete() {
  return false;
}

// List of tags, and FGs they belong to, for initializing dimensions module
int MultiframeObject::initializeDimensions(std::vector<std::pair<DcmTag,DcmTag> > dimTagList){
  OFString dimUID;

  dimensionsModule.clearData();

  dimUID = dcmqi::Helper::generateUID();
  for(int i=0;i<dimTagList.size();i++){
    std::pair<DcmTag,DcmTag> dimTagPair = dimTagList[i];
    CHECK_COND(dimensionsModule.addDimensionIndex(dimTagPair.first, dimUID, dimTagPair.second,
    dimTagPair.first.getTagName()));
  }
  return EXIT_SUCCESS;
}

int MultiframeObject::initializePixelMeasuresFG(){
  string pixelSpacingStr, sliceSpacingStr;

  pixelSpacingStr = dcmqi::Helper::floatToStrScientific(volumeGeometry.spacing[0])+
      "\\"+dcmqi::Helper::floatToStrScientific(volumeGeometry.spacing[1]);
  sliceSpacingStr = dcmqi::Helper::floatToStrScientific(volumeGeometry.spacing[2]);

  CHECK_COND(pixelMeasuresFG.setPixelSpacing(pixelSpacingStr.c_str()));
  CHECK_COND(pixelMeasuresFG.setSpacingBetweenSlices(sliceSpacingStr.c_str()));
  CHECK_COND(pixelMeasuresFG.setSliceThickness(sliceSpacingStr.c_str()));

  return EXIT_SUCCESS;
}

int MultiframeObject::initializePlaneOrientationFG() {
  planeOrientationPatientFG.setImageOrientationPatient(
      dcmqi::Helper::floatToStrScientific(volumeGeometry.rowDirection[0]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.rowDirection[1]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.rowDirection[2]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.columnDirection[0]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.columnDirection[1]).c_str(),
      dcmqi::Helper::floatToStrScientific(volumeGeometry.columnDirection[2]).c_str()
  );
  return EXIT_SUCCESS;
}

ContentItemMacro* MultiframeObject::initializeContentItemMacro(CodeSequenceMacro conceptName,
                                                               CodeSequenceMacro conceptCode){
  ContentItemMacro* item = new ContentItemMacro();
  CodeSequenceMacro* concept = new CodeSequenceMacro(conceptName);
  CodeSequenceMacro* value = new CodeSequenceMacro(conceptCode);

  if (!item || !concept || !value)
  {
    return NULL;
  }

  item->getEntireConceptNameCodeSequence().push_back(concept);
  item->getEntireConceptCodeSequence().push_back(value);
  item->setValueType(ContentItemMacro::VT_CODE);

  return EXIT_SUCCESS;
}

// populates slice2frame vector that maps each of the volume slices to the set of frames that
// are considered as derivation dataset
// TODO: this function assumes that all of the derivation datasets are images, which is probably ok
int MultiframeObject::mapVolumeSlicesToDICOMFrames(ImageVolumeGeometry& volume,
                                                   const vector<DcmDataset*> dcmDatasets,
                                                   vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > &slice2frame){
  slice2frame.resize(volume.extent[2]);

  for(int d=0;d<dcmDatasets.size();d++){
    Uint32 numFrames;
    DcmDataset* dcm = dcmDatasets[d];
    if(dcm->findAndGetUint32(DCM_NumberOfFrames, numFrames).good()){
      // this is a multiframe object
      for(int f=0;f<numFrames;f++){
        dcmqi::DICOMFrame frame(dcm,f+1);
        vector<int> intersectingSlices = findIntersectingSlices(volume, frame);

        for(int s=0;s<intersectingSlices.size();s++) {
          slice2frame[s].insert(frame);
          if (!s)
            this->insertDerivationSeriesInstance(frame.getSeriesUID(), frame.getInstanceUID());
        }
      }
    } else {
      dcmqi::DICOMFrame frame(dcm);
      vector<int> intersectingSlices = findIntersectingSlices(volume, frame);

      for(int s=0;s<intersectingSlices.size();s++) {
        slice2frame[s].insert(frame);
        if (!s)
          this->insertDerivationSeriesInstance(frame.getSeriesUID(), frame.getInstanceUID());
      }
    }
  }

  return EXIT_SUCCESS;
}


int MultiframeObject::addDerivationItemToDerivationFG(FGDerivationImage* fgder, set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> derivationFrames,
                                                         CodeSequenceMacro purposeOfReferenceCode,
                                                         CodeSequenceMacro derivationCode){
  DerivationImageItem *derimgItem;
  // TODO: check what we should do with DerivationDescription
  CHECK_COND(fgder->addDerivationImageItem(derivationCode,"",derimgItem));

  OFVector<DcmDataset*> siVector;
  std::vector<dcmqi::DICOMFrame> frameVector;

  for(set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare>::const_iterator sIt=derivationFrames.begin();
      sIt!=derivationFrames.end();++sIt) {
    siVector.push_back((*sIt).getDataset());
    frameVector.push_back(*sIt);
  }

  OFVector<SourceImageItem*> srcimgItems;
  CHECK_COND(derimgItem->addSourceImageItems(siVector, purposeOfReferenceCode, srcimgItems));

  // iterate over source image items (assuming they are in the same order as in siVector!), and initialize
  // frame number, if applicable
  unsigned siItemCnt=0;
  for(OFVector<SourceImageItem*>::iterator vIt=srcimgItems.begin();
      vIt!=srcimgItems.end();++vIt,++siItemCnt) {
    // TODO: when multuple frames from the same instance are used, they should be referenced within a single
    //  ImageSOPInstanceReferenceMacro. There would need to be another level of checks over all of the frames
    //  that are mapped to the given slice to identify those that are from the same instance, and populate the
    //  list of frames. I can't think of any use case where this would be immediately important, but if we ever
    //  use multiframe for DCE/DWI, with all of the temporal/b-value frames stored in a single object, there will
    //  be multiple frames used to derive a single frame in a parametric map, for example.
    if(frameVector[siItemCnt].getFrameNumber()) {
      OFVector<Uint16> frameNumbersVector;
      ImageSOPInstanceReferenceMacro &instRef = (*vIt)->getImageSOPInstanceReference();
      frameNumbersVector.push_back(frameVector[siItemCnt].getFrameNumber());
      instRef.setReferencedFrameNumber(frameNumbersVector);
    }
  }

  return EXIT_SUCCESS;
}

std::vector<int> MultiframeObject::findIntersectingSlices(ImageVolumeGeometry &volume, dcmqi::DICOMFrame &frame) {
  std::vector<int> intersectingSlices;
  // for now, adopt a simple strategy that maps origin of the frame to index, and selects the slice corresponding
  //  to this index as the intersecting one
  ImageVolumeGeometry::DummyImageType::Pointer itkvolume = volume.getITKRepresentation<ImageVolumeGeometry::DummyImageType>();
  ImageVolumeGeometry::DummyImageType::PointType point;
  ImageVolumeGeometry::DummyImageType::IndexType index;
  vnl_vector<double> frameIPP = frame.getFrameIPP();
  point[0] = frameIPP[0];
  point[1] = frameIPP[1];
  point[2] = frameIPP[2];

  if(itkvolume->TransformPhysicalPointToIndex(point, index)) {
    intersectingSlices.push_back(index[2]);
  }

  return intersectingSlices;
}

void MultiframeObject::insertDerivationSeriesInstance(string seriesUID, string instanceUID){
  if(derivationSeriesToInstanceUIDs.find(seriesUID) == derivationSeriesToInstanceUIDs.end())
    derivationSeriesToInstanceUIDs[seriesUID] = std::set<string>();
  derivationSeriesToInstanceUIDs[seriesUID].insert(instanceUID);
}

int MultiframeObject::initializeCommonInstanceReferenceModule(IODCommonInstanceReferenceModule &commref, vector<set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > &slice2frame){

  // map individual series UIDs to the list of instance UIDs - we need to have this organization
  // to populate Common instance reference module
  std::map<std::string, std::set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> > series2frame;
  for(int slice=0;slice!=slice2frame.size();slice++){
    for(set<dcmqi::DICOMFrame, dcmqi::DICOMFrame_compare>::const_iterator frameI=slice2frame[slice].begin();
        frameI!=slice2frame[slice].end();++frameI){
      dcmqi::DICOMFrame frame = *frameI;
      if(series2frame.find(frame.getSeriesUID()) == series2frame.end()){
        std::set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> setOfInstances;
        setOfInstances.insert(frame);
        series2frame[frame.getSeriesUID()] = setOfInstances;
      } else {
        series2frame[frame.getSeriesUID()].insert(frame);
      }
    }
  }

  // create a new ReferencedSeriesItem for each series, and populate with instances
  OFVector<IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem*> &refseries = commref.getReferencedSeriesItems();
  for(std::map<std::string, std::set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare> >::const_iterator mIt=series2frame.begin(); mIt!=series2frame.end();++mIt){
    // TODO: who is supposed to de-allocate this?
    IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem* refseriesItem = new IODSeriesAndInstanceReferenceMacro::ReferencedSeriesItem;
    refseriesItem->setSeriesInstanceUID(mIt->first.c_str());
    OFVector<SOPInstanceReferenceMacro*> &refinstances = refseriesItem->getReferencedInstanceItems();

    for(std::set<dcmqi::DICOMFrame,dcmqi::DICOMFrame_compare>::const_iterator sIt=mIt->second.begin();sIt!=mIt->second.end();++sIt){
      dcmqi::DICOMFrame frame = *sIt;
      SOPInstanceReferenceMacro *refinstancesItem = new SOPInstanceReferenceMacro();
      CHECK_COND(refinstancesItem->setReferencedSOPClassUID(frame.getClassUID().c_str()));
      CHECK_COND(refinstancesItem->setReferencedSOPInstanceUID(frame.getInstanceUID().c_str()));
      refinstances.push_back(refinstancesItem);
    }

    refseries.push_back(refseriesItem);
  }

  return EXIT_SUCCESS;
}

int MultiframeObject::getImageDirections(FGInterface& fgInterface, DirectionType &dir){
  // TODO: handle the situation when FoR is not initialized
  OFBool isPerFrame;
  vnl_vector<double> rowDirection(3), colDirection(3);

  FGPlaneOrientationPatient *planorfg = OFstatic_cast(FGPlaneOrientationPatient*,
                                                      fgInterface.get(0, DcmFGTypes::EFG_PLANEORIENTPATIENT, isPerFrame));
  if(!planorfg){
    cerr << "Plane Orientation (Patient) is missing, cannot parse input " << endl;
    return EXIT_FAILURE;
  }

  if(planorfg->getImageOrientationPatient(rowDirection[0], rowDirection[1], rowDirection[2],
                                          colDirection[0], colDirection[1], colDirection[2]).bad()){
    cerr << "Failed to get orientation " << endl;
    return EXIT_FAILURE;
  }

  vnl_vector<double> sliceDirection = vnl_cross_3d(rowDirection, colDirection);
  sliceDirection.normalize();

  for(int i=0;i<3;i++){
    dir[i][0] = rowDirection[i];
    dir[i][1] = colDirection[i];
    dir[i][2] = sliceDirection[i];
  }

  cout << "Row direction: " << rowDirection << endl;
  cout << "Col direction: " << colDirection << endl;
  cout << "Z direction  : " << sliceDirection << endl;

  return EXIT_SUCCESS;
}

int MultiframeObject::computeVolumeExtent(FGInterface& fgInterface, vnl_vector<double> &sliceDirection,
                                          PointType &imageOrigin, double &sliceSpacing, double &sliceExtent) {
  // Size
  // Rows/Columns can be read directly from the respective attributes
  // For number of slices, consider that all segments must have the same number of frames.
  //   If we have FoR UID initialized, this means every segment should also have Plane
  //   Position (Patient) initialized. So we can get the number of slices by looking
  //   how many per-frame functional groups a segment has.

  vector<double> originDistances;
  map<OFString, double> originStr2distance;
  map<OFString, unsigned> frame2overlap;
  double minDistance = 0.0;

  sliceSpacing = 0;

  // Determine ordering of the frames, keep mapping from ImagePositionPatient string
  //   to the distance, and keep track (just out of curiosity) how many frames overlap
  vnl_vector<double> refOrigin = getFrameOrigin(fgInterface, 0);

  unsigned numFrames = fgInterface.getNumberOfFrames();

  for(size_t frameId=0;frameId<numFrames;frameId++){
    OFBool isPerFrame;
    FGPlanePosPatient *planposfg = OFstatic_cast(FGPlanePosPatient*,
                                                 fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));

    if(!planposfg){
      cerr << "PlanePositionPatient is missing" << endl;
      return EXIT_FAILURE;
    }

    if(!isPerFrame){
      cerr << "PlanePositionPatient is required for each frame!" << endl;
      return EXIT_FAILURE;
    }

    vnl_vector<double> sOrigin(3);
    sOrigin = getFrameOrigin(planposfg);
    std::ostringstream sstream;
    sstream << sOrigin[0] << "/" << sOrigin[1] << "/" << sOrigin[2];
    OFString sOriginStr = sstream.str().c_str();

    // check if this frame has already been encountered
    if(originStr2distance.find(sOriginStr) == originStr2distance.end()){
      vnl_vector<double> difference(3);
      difference[0] = sOrigin[0]-refOrigin[0];
      difference[1] = sOrigin[1]-refOrigin[1];
      difference[2] = sOrigin[2]-refOrigin[2];

      double dist = dot_product(difference, sliceDirection);
      frame2overlap[sOriginStr] = 1;
      originStr2distance[sOriginStr] = dist;
      assert(originStr2distance.find(sOriginStr) != originStr2distance.end());
      originDistances.push_back(dist);

      if(frameId == 0 || dist < minDistance){
        minDistance = dist;
        imageOrigin[0] = sOrigin[0];
        imageOrigin[1] = sOrigin[1];
        imageOrigin[2] = sOrigin[2];
      }
    } else {
      frame2overlap[sOriginStr]++;
    }
  }

  // it IS possible to have a segmentation object containing just one frame!
  if(numFrames>1){
    // WARNING: this should be improved further. Spacing should be calculated for
    //  consecutive frames of the individual segment. Right now, all frames are considered
    //  indiscriminately. Question is whether it should be computed at all, considering we do
    //  not have any information about whether the 2 frames are adjacent or not, so perhaps we should
    //  always rely on the declared spacing, and not even try to compute it?
    // TODO: discuss this with the QIICR team!

    // sort all unique distances, this will be used to check consistency of
    //  slice spacing, and also to locate the slice position from ImagePositionPatient
    //  later when we read the segments
    sort(originDistances.begin(), originDistances.end());

    sliceSpacing = fabs(originDistances[0]-originDistances[1]);

    for(size_t i=1;i<originDistances.size(); i++){
      float dist1 = fabs(originDistances[i-1]-originDistances[i]);
      float delta = sliceSpacing-dist1;
      cout << "Spacing between frame " << i-1 << " and " << i << ": " << dist1 << endl;
    }

    sliceExtent = fabs(originDistances[0]-originDistances[originDistances.size()-1]);
    unsigned overlappingFramesCnt = 0;
    for(map<OFString, unsigned>::const_iterator it=frame2overlap.begin();
        it!=frame2overlap.end();++it){
      if(it->second>1)
        overlappingFramesCnt++;
    }

    cout << "Total frames: " << numFrames << endl;
    cout << "Total frames with unique IPP: " << originDistances.size() << endl;
    cout << "Total overlapping frames: " << overlappingFramesCnt << endl;
    cout << "Origin: " << imageOrigin << endl;
  }

  return EXIT_SUCCESS;
}

vnl_vector<double> MultiframeObject::getFrameOrigin(FGInterface &fgInterface, int frameId) const {
  OFBool isPerFrame;
  FGPlanePosPatient *planposfg = OFstatic_cast(FGPlanePosPatient*,
                                               fgInterface.get(frameId, DcmFGTypes::EFG_PLANEPOSPATIENT, isPerFrame));
  return getFrameOrigin(planposfg);
}

vnl_vector<double> MultiframeObject::getFrameOrigin(FGPlanePosPatient *planposfg) const {
  vnl_vector<double> origin(3);
  if(!planposfg->getImagePositionPatient(origin[0], origin[1], origin[2]).good()){
    cerr << "Failed to read patient position" << endl;
    throw -1;
  }
  return origin;
}

int MultiframeObject::getDeclaredImageSpacing(FGInterface &fgInterface, SpacingType &spacing){
  OFBool isPerFrame;
  FGPixelMeasures *pixelMeasures = OFstatic_cast(FGPixelMeasures*,
                                                 fgInterface.get(0, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame));
  if(!pixelMeasures){
    cerr << "Pixel measures FG is missing!" << endl;
    return EXIT_FAILURE;
  }

  pixelMeasures->getPixelSpacing(spacing[0], 0);
  pixelMeasures->getPixelSpacing(spacing[1], 1);

  Float64 spacingFloat;
  if(pixelMeasures->getSpacingBetweenSlices(spacingFloat,0).good() && spacingFloat != 0){
    spacing[2] = spacingFloat;
  } else if(pixelMeasures->getSliceThickness(spacingFloat,0).good() && spacingFloat != 0){
    // SliceThickness can be carried forward from the source images, and may not be what we need
    // As an example, this ePAD example has 1.25 carried from CT, but true computed thickness is 1!
    cerr << "WARNING: SliceThickness is present and is " << spacingFloat << ". using it!" << endl;
    spacing[2] = spacingFloat;
  }
  return EXIT_SUCCESS;
}


int MultiframeObject::initializeSeriesSpecificAttributes(IODGeneralSeriesModule& generalSeriesModule,
                                                         IODGeneralImageModule& generalImageModule){

  // TODO: error checks
  string bodyPartExamined;
  if(metaDataJson.isMember("BodyPartExamined")){
    bodyPartExamined = metaDataJson["BodyPartExamined"].asCString();
  }
  if(derivationDcmDatasets.size() && bodyPartExamined.empty()) {
    OFString bodyPartStr;
    DcmDataset *srcDataset = derivationDcmDatasets[0];
    if(srcDataset->findAndGetOFString(DCM_BodyPartExamined, bodyPartStr).good()) {
      if (!bodyPartStr.empty())
        bodyPartExamined = bodyPartStr.c_str();
    }
  }

  if(!bodyPartExamined.empty())
    generalSeriesModule.setBodyPartExamined(bodyPartExamined.c_str());

  // SeriesDate/Time should be of when parametric map was taken; initialize to when it was saved
  {
    OFString contentDate, contentTime;
    DcmDate::getCurrentDate(contentDate);
    DcmTime::getCurrentTime(contentTime);

    // TODO: AcquisitionTime
    generalSeriesModule.setSeriesDate(contentDate.c_str());
    generalSeriesModule.setSeriesTime(contentTime.c_str());
    generalImageModule.setContentDate(contentDate.c_str());
    generalImageModule.setContentTime(contentTime.c_str());
  }

  generalSeriesModule.setSeriesDescription(metaDataJson["SeriesDescription"].asCString());
  generalSeriesModule.setSeriesNumber(metaDataJson["SeriesNumber"].asCString());

  return EXIT_SUCCESS;
}