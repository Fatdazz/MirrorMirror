#include "ofApp.h"



//--------------------------------------------------------------
void ofApp::setup(){
  //ofSetLogLevel(OF_LOG_VERBOSE);
    ofDisableArbTex();
    ofEnableSmoothing();
    ofEnableAlphaBlending();

  // init kinect
#if USE_KINECT_2
  kinect.open();

#endif

  // init Face traker
  trackerFace.setup();

    

#if USE_KINECT_2
  
  imageColor.allocate(width_kinect, height_kinect, ofImageType::OF_IMAGE_COLOR);


  fboColorMaskAndBackground.allocate(width_kinect, height_kinect, GL_RGB);
  fboColorMaskAndBackground.begin();
  ofClear(255,255,255, 0);
  fboColorMaskAndBackground.end();
#endif
    
  ofFbo::Settings fboS;
  fboS.width = win_width;
  fboS.height = win_height;
  fboS.internalformat = GL_RGB;

  imageBlur.setup(fboS , 3 , 50.0f);
  imageBlur.setNumBlurOverlays(1);
  imageBlur.setBlurOffset(0.2);
  imageBlur.setBlurPasses(2);
    
  imageBlur.setBlurOffset(2.0f);
  imageBlur.setBlurPasses(2);

  nearThreshold = 255;
  farThreshold = 236;
  //farThreshold =0;
  colormap.setMapFromName("gist_heat");
    
  int bufferSize = 256;
  soundStream.setup(this, 2, 1, 48000, bufferSize, 32);
  rec = play = false;
  bufferCounter = 0;
  faceAnimationPtr = nullptr;

  ofDirectory dir(ofToDataPath("."));
  dir.allowExt("cereal");
  dir.listDir();
  numFiles = dir.getFiles().size();

  std::cout << numFiles << "\n";

  fbo.allocate(win_width, win_height, GL_RGB);
  fbo.begin();
  ofClear(0);
  fbo.end();

}

//--------------------------------------------------------------
void ofApp::update() {

  ofSetWindowTitle("FPS: " + std::to_string(ofGetFrameRate()));
  
  //imageBlur.setBlurOffset(200 * ofMap(mouseX, 0, ofGetWidth(), 0, 1, true));
  //imageBlur.setBlurPasses(10. * ofMap(mouseY, 0, ofGetHeight(), 1, 0, true));
  kinect.update();
  if (kinect.isFrameNew()) {

    fbo.begin();
  ofClear(0);
  fbo.end();

#if USE_KINECT_2

    imageColor.setFromPixels(kinect.getRgbPixels());
    imageColor.resize(win_width, win_height);
    
    //
    fbo.begin();
    filter.begin();

    ofPushMatrix();
    imageColor.draw(0, 0);
    ofPopMatrix();

    filter.end();
    fbo.end();

    
#endif

    if (!trackerFace.isThreadRunning()) {
      trackerFace.startThread();
    }

    ofPixels pixels;

    fbo.getTexture().readToPixels(pixels);
    grayImage.setFromPixels(pixels);

    trackerFace.update(ofxCv::toCv(imageColor));
    

    //colormap.apply(grayImage, imageColor);

    // Buffer Face
    bufferAnimation = trackerFace.getObjectMesh();

    animation = trackerFace.getImageMesh();
    for (int i = 0; i < animation.getVertices().size(); i++) {
      animation.addTexCoord(ofVec2f(animation.getVertices()[i].x / 2, animation.getVertices()[i].y / 2));
    }

    if (faceAnimationPtr != nullptr && play) {

      //unique_lock<mutex> lock(audioMutex);
      if (trackerFace.getFound()) {
	ofMesh lulu = faceAnimationPtr->face[bufferCounter];
	for (int i = 0; i < lulu.getVertices().size(); i++) {
	  ofVec3f point = lulu.getVertices()[i];
	  ofMatrix4x4 transformation;
	  float scale = trackerFace.getScale();
	  transformation.makeScaleMatrix(ofVec3f(scale, scale, 0));
	  transformation = transformation * trackerFace.getRotationMatrix();
	  point = point * transformation;
	  point = ofVec3f(point.x + trackerFace.getPosition().x, point.y + trackerFace.getPosition().y, 0);
	  lulu.setVertex(i, point);
	}

	// mouth
	for (int i = 48; i < 66; i++) {
	  animation.setVertex(i, lulu.getVertices()[i]);
	}

	// jaw
	for (int i = 4; i < 14; i++) {
	  animation.setVertex(i, lulu.getVertices()[i]);
	}
      }
    }
    fboColorMaskAndBackground.begin();
    imageColor.draw(0, 0);
    //grayFboImage.draw(0, 0, win_gray_width_kinect*2, win_gray_height_kinect*2);

    imageColor.bind();
    ofPushMatrix();
    //ofScale(0.5, 0.5);
    animation.draw();
    ofPopMatrix();
    //animationMouth.draw();
    imageColor.unbind();
    fboColorMaskAndBackground.end();
  }
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofSetWindowTitle(ofToString(ofGetFrameRate()));

    
  if (!debug) {
    //imageBlur.drawBlurFbo();
    fboColorMaskAndBackground.draw(0, 0, width_kinect, height_kinect);
    grayImage.draw(0, 0, width_kinect, height_kinect);
  } else {

    ofSetColor(ofColor::white);
        
    // image 1
    ofPushMatrix();
    ofTranslate(0, 0);

    ofScale(0.5, 0.5);
    imageColor.draw(0, 0);
    ofTranslate(width_kinect, 0);
    grayImage.draw(0, 0);

    ofSetColor(ofColor::blue);
    trackerFace.getImageMesh().drawWireframe();
    ofSetColor(ofColor::white);

    ofSetColor(ofColor::white);
    ofPopMatrix();

    // image 2

    fboColorMaskAndBackground.draw(0, height_kinect);

    
  }


}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

  switch (key) {
  case 'e':
    cout << "BlurOffse: " << imageBlur.getBlurOffset() << " BlurPasses: " <<imageBlur.getBlurPasses() << endl;
    break;
  case 'o':
    farThreshold++;
    cout << "far: " << farThreshold <<" near: "<<nearThreshold << endl;
    break;
  case 'i':
    farThreshold--;
    cout << "far: " << farThreshold <<" near: "<<nearThreshold << endl;
    break;
  case 'r':
    cout << " Rec " << endl;
    if (!play && !rec && trackerFace.getFound()) {
      faceAnimationVect.emplace_back();
      faceAnimationPtr = &faceAnimationVect.at(faceAnimationVect.size()-1);
      rec = !rec;
    }
    break;
  case 'p':
    cout << " Play " << endl;
    if (rec) {
      rec = false;
    }
    play=!play;
    break;
  case ' ':
    debug = !debug;
    break;
  case 's': {
      ofDirectory dir(".");
      dir.allowExt("cereal");

      auto path = "soundbuffer"+std::to_string(numFiles);
      std::ofstream os(ofToDataPath(path+".cereal"), std::ios::binary);
      dir.createDirectory(path + "Mesh");
      cereal::PortableBinaryOutputArchive archive( os );
      std::cout << "save\n";
      archive( faceAnimationVect.back().soundBuffer );

      int counter = 0;
      for (auto& mesh : faceAnimationVect.back().face) {
	mesh.save(path + "Mesh/mesh"+std::to_string(counter)+".ply");
	counter++;
      }

      dir.listDir();
      numFiles = dir.getFiles().size();
    }
    break;

    case 'l': {
      bool found = false;
      if (numFiles == 0) break;
      do {
	auto path = ofToDataPath("soundbuffer"+std::to_string((int)ofRandom(0,numFiles+1)));
	std::ifstream os(path+".cereal", std::ios::binary);
	if (os.is_open()) {
	  found = true;
	  cereal::PortableBinaryInputArchive archive( os );
	  std::cout << "load\n";
	  ofSoundBufferCereal faceAnimationTmp;
	  archive( faceAnimationTmp );
	  if (faceAnimationVect.size() == 0) {
	    faceAnimationVect.emplace_back();
	  }
	  faceAnimationVect.back().soundBuffer.swap(faceAnimationTmp);
	  std::cout << "toto\n";
	  ofDirectory dir(path+"Mesh/");
	  dir.listDir();
	  dir.sort();

	  faceAnimationVect.back().face.clear();
	  for (auto& file : dir.getFiles()) {
	    std::cout << path+"Mesh/"+file.getFileName() << "\n";
	    faceAnimationVect.back().face.emplace_back();
	    faceAnimationVect.back().face.back().load(path+"Mesh/"+file.getFileName());
	  }
	}
      } while (!found);
    }
      break;
  }
}
//--------------------------------------------------------------
void ofApp::audioIn( ofSoundBuffer& buffer ){
  /*
    #if USE_KINECT_2
    // L'addon kinect 2 n'a pas de fonction pour si elle est connecté
    // on se repere avec l'arrivee de nouvelles frames
    if (rec && faceAnimationPtr != nullptr && kinect.isFrameNew()) {
    #else
    if (rec && faceAnimationPtr != nullptr && kinect.isConnected()) {
    #endif
  */
  if (rec) {
    // Enregistrement Audio
    faceAnimationPtr->soundBuffer.append(buffer);
        
    // Enregistre les 68 pt du face tracking
    unique_lock<mutex> lock(audioMutex);
    faceAnimationPtr->face.push_back(bufferAnimation);
  }
}
//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer &outBuffer){
  auto nChannel = outBuffer.getNumChannels();
  if (play && faceAnimationPtr != nullptr && bufferCounter < (faceAnimationPtr->soundBuffer.size()/soundStream.getBufferSize())-1) {
    for (int i = 0; i < outBuffer.getNumFrames(); i++) {
      // Lecture Audio
      auto sample = faceAnimationPtr->soundBuffer.getBuffer()[i + bufferCounter * outBuffer.getNumFrames()];
      outBuffer.getSample(i, 0) = sample;
      outBuffer.getSample(i, 1) = sample;
    }
    bufferCounter++;
  } else {
    play = false;
    bufferCounter = 0;
  }
}
//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}
//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}
//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}
//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}
//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}
//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}
//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}
//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}
//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}
//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
//--------------------------------------------------------------
void ofApp::tranposeRotation(ofMatrix4x4 *_Matrix){
    
  /*
   * This changes the matrix.
   * [ a b c ]T    [ a d g ]
   * [ d e f ]  =  [ b e h ]
   * [ g h i ]     [ c f i ]
   b += d; d = b - d; b -= d; //swap b and d
   c += g; g = c - g; c -= g; //swap c and g
   f += h; h = f - h; f -= h; //swap f and h
  */
    
  // rotation
  _Matrix->_mat[0][1]+= _Matrix->_mat[1][0]; _Matrix->_mat[1][0] = _Matrix->_mat[0][1] -_Matrix->_mat[1][0]; _Matrix->_mat[0][1]-= _Matrix->_mat[1][0];
    
  _Matrix->_mat[0][2]+= _Matrix->_mat[2][0]; _Matrix->_mat[2][0] = _Matrix->_mat[0][2] -_Matrix->_mat[2][0]; _Matrix->_mat[0][2]-= _Matrix->_mat[2][0];
    
  _Matrix->_mat[2][1]+= _Matrix->_mat[1][2]; _Matrix->_mat[1][2] = _Matrix->_mat[2][1] -_Matrix->_mat[1][2]; _Matrix->_mat[2][1]-= _Matrix->_mat[1][2];
 
  // translation
  _Matrix->_mat[3][0]= -_Matrix->_mat[3][0]; _Matrix->_mat[3][1]= -_Matrix->_mat[3][1]; _Matrix->_mat[3][2]= -_Matrix->_mat[3][2];
}

