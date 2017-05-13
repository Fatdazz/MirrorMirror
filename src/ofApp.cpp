#include "ofApp.h"



//--------------------------------------------------------------
void ofApp::setup(){
  //ofSetLogLevel(OF_LOG_VERBOSE);

#if USE_KINECT_2
  // Shader MultiKinectV2 profondeur et ir
  depthShader.setupShaderFromSource(GL_FRAGMENT_SHADER, depthFragmentShader);
  depthShader.linkProgram();

  irShader.setupShaderFromSource(GL_FRAGMENT_SHADER, irFragmentShader);
  irShader.linkProgram();
#endif

  // init kinect
#if USE_KINECT_2
  kinect.open();
  kinect.start();

  gr.setup(kinect.getProtonect(), 2);
#endif
    
  // init Face traker
  trackerFace.setup();
    
  // init contourFinder
  contourFinder.setMinAreaRadius(90); // à determiné <===============================
  contourFinder.setMaxAreaRadius(300);
  contourFinder.setUseTargetColor(false);
  contourFinder.setThreshold(0);

#if USE_KINECT_2
  imageGray.allocate(win_gray_width_kinect, win_gray_height_kinect, ofImageType::OF_IMAGE_GRAYSCALE);
  imageColor.allocate(win_gray_width_kinect, win_gray_height_kinect, ofImageType::OF_IMAGE_COLOR);

  fboGray.allocate(win_gray_width_kinect, win_gray_height_kinect, GL_RGB);
  fboGray.begin();
  ofClear(255,255,255, 0);
  fboGray.end();

  fboColorMaskAndBackground.allocate(win_gray_width_kinect, win_gray_height_kinect, GL_RGB);
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

  colormap.setMapFromName("gist_heat");
    
  int bufferSize = 256;
  soundStream.setup(this, 2, 1, 48000, bufferSize, 32);
  rec = play = false;
  bufferCounter = 0;
  faceAnimationPtr = nullptr;
}

//--------------------------------------------------------------
void ofApp::update(){
    
  //imageBlur.setBlurOffset(200 * ofMap(mouseX, 0, ofGetWidth(), 0, 1, true));
  //imageBlur.setBlurPasses(10. * ofMap(mouseY, 0, ofGetHeight(), 1, 0, true));
  kinect.update();
  if (kinect.isFrameNew()) {

#if USE_KINECT_2

    // import textures kinect
    colorTex0.loadData(kinect.getColorPixelsRef());
    depthTex0.loadData(kinect.getDepthPixelsRef());

    // Calcul image superpos√© profondeur et couleur
    gr.update(depthTex0, colorTex0, true);
    gr.getRegisteredTexture().readToPixels(tmpPixels);
    imgGr.setFromPixels(tmpPixels);

    // envoie superpos√© image au face tracker (thread)
    trackerFace.update(ofxCv::toCv(imgGr));

    //depthTex0.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST); <-- Regarder pour lissage
    //gr.update(depthTex0, colorTex0, true);


    imageGray.setFromPixels(kinect.getDepthPixelsRef());

#endif

    contourFinder.findContours(imageGray);
        
    if (contourFinder.getPolylines().size() > 0 && !trackerFace.isThreadRunning()) {
      trackerFace.startThread();
      cout << "start" << endl;
    }

    if (contourFinder.getPolylines().size() == 0 && trackerFace.isThreadRunning()) {
      //trackerFace.stopThread();
      cout << "stop" << endl;
    }

    //imageGray.resize(win_gray_width_kinect*2, win_gray_height_kinect*2);
    //imageGray.update();

    fboGray.begin();
    ofClear(0);
    depthShader.begin();
    depthTex0.draw(0, 0);
    //depthShader.setUniformTexture("tex", imageGray.getTexture(), 0);
    if (trackerFace.getFound() && animation.getVertices().size() > 0) {
      //ofSetColor(ofColor::green);
      const ofMesh& m = animation;
      //std::cout << m.getVertices()[48] << " " << m.getVertices()[60] << "\n";
      //ofDrawCircle(m.getVertices()[48]/2, 5);
      //ofDrawCircle(m.getVertices()[54]/2, 5);
      /*
      ofSetColor(ofColor::black);
      for (unsigned int i = 4; i < 13; i++) {
	ofDrawCircle(m.getVertices()[i]/2, 5);
      }
      */

    }

    depthShader.end();
    fboGray.end();

    //std::cout << imageGray.getImageType() << "\n";

    //fboGray.getTexture().readToPixels(tmpPixels);
    //imageGray.setFromPixels(tmpPixels);
    //imageGray.update();
    fboGray.getTexture().readToPixels(tmpPixels);
    grayFboImage.setFromPixels(tmpPixels);
    grayFboImage.setImageType(OF_IMAGE_GRAYSCALE);
    //grayFboImage.update();
    colormap.apply(grayFboImage, imageColor);
    //imageColor.update();

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
    imageColor.resize(win_gray_width_kinect*2, win_gray_height_kinect*2);
    imageColor.bind();
    ofPushMatrix();
    ofScale(0.5, 0.5);
    animation.draw();
    ofPopMatrix();
    //animationMouth.draw();
    imageColor.unbind();
    imageColor.resize(win_gray_width_kinect  , win_gray_height_kinect  );
    fboColorMaskAndBackground.end();
  }
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofSetWindowTitle(ofToString(ofGetFrameRate()));

    
  if (!debug) {
    //imageBlur.drawBlurFbo();
    fboColorMaskAndBackground.draw(0, 0, win_gray_width_kinect*2, win_gray_height_kinect*2);
  } else {

    ofSetColor(ofColor::white);
        
    // image 1
    ofPushMatrix();
    ofTranslate(0, 0);
    ofScale(0.5, 0.5);
    //imageGray.draw(0, 0);
    //fboGray.draw(0, 0, win_gray_width_kinect*2, win_gray_height_kinect*2);
    grayFboImage.draw(0, 0, win_gray_width_kinect*2, win_gray_height_kinect*2);

    ofSetColor(ofColor::blue);
    trackerFace.getImageMesh().drawWireframe();
    ofSetColor(ofColor::white);

    ofSetColor(ofColor::white);
    ofPopMatrix();

    // image 2
    ofPushMatrix();
    ofTranslate(win_width/2, 0);
    ofScale(0.5, 0.5);
    fboColorMaskAndBackground.draw(0, 0, win_gray_width_kinect*2, win_gray_height_kinect*2);
    ofPopMatrix();
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
  }
}
//--------------------------------------------------------------
void ofApp::audioIn( ofSoundBuffer& buffer ){
  /*
    #if USE_KINECT_2
    // L'addon kinect 2 n'a pas de fonction pour si elle est connect√©
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

