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
  contourFinder.setMinAreaRadius(80); // à determiné <===============================
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

  nearThreshold = 255;
  farThreshold = 236;
  //farThreshold =0;
  colormap.setMapFromName("gist_heat");
    
  int bufferSize = 256;
  soundStream.setup(this, 2, 1, 48000, bufferSize, 32);
  rec = play = false;
  bufferCounter = 0;
  faceAnimationPtr = nullptr;


  ofFbo::Settings s;
  s.width = win_width/2;
  s.height = win_height/2;
  s.internalformat = GL_RGBA;

  s.maxFilter = GL_LINEAR;
  s.numSamples = 0;
  s.numColorbuffers = 1;
  s.useDepth = false;
  s.useStencil = false;

  blur.setup(s, false);

  ofDirectory dir(ofToDataPath("."));
  dir.allowExt("cereal");
  dir.listDir();
  numFiles = dir.getFiles().size();

  std::cout << numFiles << "\n";

  for (auto f : dir.getFiles()) {
    std::cout << f.getFileName() << "\n";
  }

  for (unsigned int i = 0; i < numFiles; i++) {
    auto path = ofToDataPath("soundbuffer"+std::to_string(i));
    std::ifstream os(path+".cereal", std::ios::binary);
    if (os.is_open()) {
      cereal::PortableBinaryInputArchive archive( os );
      ofSoundBufferCereal faceAnimationTmp;
      archive( faceAnimationTmp );
      finalAnimations.emplace_back();
      finalAnimations.back().soundBuffer.swap(faceAnimationTmp);
      ofDirectory dir(path+"Mesh/");
      dir.listDir();
      dir.sort();
      
      finalAnimations.back().face.clear();
      for (auto& file : dir.getFiles()) {
	finalAnimations.back().face.emplace_back();
	finalAnimations.back().face.back().load(path+"Mesh/"+file.getFileName());
      }
    }
  }

  randomAnimation = ofRandom(numFiles);
}
  

//--------------------------------------------------------------
void ofApp::update(){
    
  //imageBlur.setBlurOffset(200 * ofMap(mouseX, 0, ofGetWidth(), 0, 1, true));
  //imageBlur.setBlurPasses(10. * ofMap(mouseY, 0, ofGetHeight(), 1, 0, true));

  blur.blurOffset = 3;
  blur.blurPasses = 3;
  blur.numBlurOverlays = 1;
  blur.blurOverlayGain = 255;
  
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

     
    fboGray.begin();
    ofClear(0);
    depthShader.begin();
    depthTex0.draw(0, 0);
    depthShader.end();
    fboGray.end();


    fboGray.getTexture().readToPixels(tmpPixels);
    grayFboImage.setFromPixels(tmpPixels);
    grayFboImage.setImageType(OF_IMAGE_GRAYSCALE);
      
    temp = grayFboImage;

    contourFinder.findContours(temp);
        
    if (contourFinder.getPolylines().size() > 0 && !trackerFace.isThreadRunning()) {
      trackerFace.startThread();
      cout << "start" << endl;    
    }

    if (!debug && trackerFace.getFound()) {
      if (!incomingPerson.isStarted() && !play) {
	std::cout << "Found someone!\n";
	incomingPerson.start();
	//std::cout << "here: " << randomAnimation << "\n";
      }
      ofSetWindowTitle(ofToString(ofGetFrameRate()) + " FOUND " + std::to_string(incomingPerson.getElapsedMillis()));
    }

    if (contourFinder.getPolylines().size() == 0 && trackerFace.isThreadRunning()) {
      trackerFace.stopThread();
      cout << "stop" << endl;
      if (!debug) {
	if (!exitPerson.isStarted()) {
	  std::cout << "stop person\n";
	  exitPerson.start();
	  incomingPerson.stop();
	} else {
	  exitPerson.reset();
	}
      }
    }

    if (!debug && finalAnimations[randomAnimation].face.size()  <= bufferCounter) {
      incomingPerson.stop();
      play = false;
      timerReset = false;
    }
    

    if (!debug && contourFinder.size() > 0 && incomingPerson.getElapsedMillis() > 3000 && trackerFace.isThreadRunning()) {
      ofSetWindowTitle(ofToString(ofGetFrameRate()) + " PLAYING");
      play = true;
      timerReset = false;
    }
   
    if (!debug && exitPerson.getElapsedMillis() > 1000) {
      exitPerson.stop();
      play = false;
    }
    
    // Buffer Face
    bufferAnimation = trackerFace.getObjectMesh();

    animation = trackerFace.getImageMesh();
    //animation.enableTextures();
    for (int i = 0; i < animation.getVertices().size(); i++) {
      animation.getVertices()[i] = animation.getVertices()[i]/2;
      //animation.addTexCoord(ofVec2f(animation.getVertices()[i].x/2 , animation.getVertices()[i].y/2));
    }
      
    if (play) {

      unique_lock<mutex> lock(audioMutex);
      if (trackerFace.getFound()) {
	ofMesh lulu;
	if (debug && faceAnimationPtr) {
	  lulu = faceAnimationPtr->face[bufferCounter];
	} else {
	  lulu = finalAnimations[randomAnimation].face[bufferCounter];
	}
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
	  animation.setVertex(i, lulu.getVertices()[i]/2);
	}

	// jaw
	for (int i = 4; i < 14; i++) {
	  animation.setVertex(i, lulu.getVertices()[i]/2);
	}
      }
    }

   
    temp.resize(win_width, win_height);
      
    fboColorMaskAndBackground.begin();
    ofClear(255,255,255, 0);
    grayFboImage.draw(0, 0);
    temp.bind();
    animation.draw();
    temp.unbind();
    fboColorMaskAndBackground.end();


    blur.beginDrawScene();
    ofClear(0);
    fboColorMaskAndBackground.draw(0, 0);
    blur.endDrawScene();
    
    blur.performBlur();
    
    blur.getBlurredSceneFbo().getTexture().readToPixels(tmpPixels);
    temp.setFromPixels(tmpPixels);
    temp.setImageType(OF_IMAGE_GRAYSCALE);


    colormap.apply(temp, imageColor);
  }
}

//--------------------------------------------------------------
void ofApp::draw(){

  if (!debug) {
    //imageBlur.drawBlurFbo();
    ofPushMatrix();
    ofTranslate(position);
    ofScale(scale);
    ofPushMatrix();
    ofTranslate(position.x+imageColor.getWidth()/2, position.y+imageColor.getHeight()/2);
    ofRotate(rotation, 0, 0, 1);
    ofTranslate(-(position.x+imageColor.getWidth()/2), -(position.y+imageColor.getHeight()/2));
    imageColor.draw(0, 0, 0, win_width , win_height);
    ofPopMatrix();
    ofSetColor(ofColor::blue);
    trackerFace.getImageMesh().drawWireframe();
    ofSetColor(ofColor::white);
    ofPopMatrix();
  } else {

    ofSetColor(ofColor::white);

    // image 1
    ofPushMatrix();
    ofTranslate(0, 0);
    ofScale(0.5, 0.5);
    //imageGray.draw(0, 0);
    //fboGray.draw(0, 0, win_gray_width_kinect*2, win_gray_height_kinect*2);
    grayFboImage.draw(0, 0, win_width , win_height);

    ofSetColor(ofColor::blue);
    trackerFace.getImageMesh().drawWireframe();
    ofSetColor(ofColor::white);

    ofSetColor(ofColor::white);
    ofPopMatrix();

    // image 2
    ofPushMatrix();
    ofTranslate(win_width/2, 0);
    ofScale(0.5, 0.5);
    imageColor.draw(0, 0, win_width , win_height);
    ofPopMatrix();
      
    // image 3
    ofPushMatrix();
    ofTranslate(0, win_height/2);
    ofScale(0.5, 0.5);
    fboColorMaskAndBackground.draw(0, 0, win_width , win_height);
    ofPopMatrix();
  }

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

  switch (key) {
    /*
      case 'k':
      colorTex0.readToPixels(tmpPixels);
      saveImage.setFromPixels(tmpPixels);
      saveImage.save("img-"+std::to_string(ofGetElapsedTimeMillis())+".png");
      depthTex0.readToPixels(tmpPixels);
      saveImage.setFromPixels(tmpPixels);
      saveImage.save("img-gray-"+std::to_string(ofGetElapsedTimeMillis())+".png");
      break;
    */
  case OF_KEY_LEFT:
    position.x -= 1.0;
    break;
  case OF_KEY_RIGHT:
    position.x += 1.0;
    break;
  case OF_KEY_UP:
    position.y -= 1.0;
    break;
  case OF_KEY_DOWN:
    position.y += 1.0;
    break;
  case '1':
    rotation += 90;
    break;
  case '3':
    rotation -= 90;
    break;
  case '7':
    scale.x += 0.1;
    break;
  case '4':
    scale.x -= 0.1;
    break;
  case '9':
    scale.y += 0.1;
    break;
  case '6':
    scale.y -= 0.1;
    break;
  case 'e':
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
#if USE_Memory
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
#endif

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
  if (debug) {
    if (play && faceAnimationPtr != nullptr && bufferCounter < (faceAnimationPtr->soundBuffer.size()/soundStream.getBufferSize())-1) {
      for (int i = 0; i < outBuffer.getNumFrames(); i++) {
	// Lecture Audio
	float sample;
	sample = faceAnimationPtr->soundBuffer.getBuffer()[i + bufferCounter * outBuffer.getNumFrames()];
	outBuffer.getSample(i, 0) = sample;
	outBuffer.getSample(i, 1) = sample;
      }
      bufferCounter++;
    } else {
      play = false;
      bufferCounter = 0;
    }
  } else {
    if (play && bufferCounter < (finalAnimations[randomAnimation].soundBuffer.size()/soundStream.getBufferSize())-1) {
      for (int i = 0; i < outBuffer.getNumFrames(); i++) {
	// Lecture Audio
	float sample;
	sample = finalAnimations[randomAnimation].soundBuffer.getBuffer()[i+bufferCounter*outBuffer.getNumFrames()];
	  
	outBuffer.getSample(i, 0) = sample;
	outBuffer.getSample(i, 1) = sample;
      }
      bufferCounter++;
    }  else {
      
      if (!timerReset) {
	timerReset = true;
	incomingPerson.stop();
	randomAnimation = ofRandom(numFiles);
      }
      
      play = false;
      bufferCounter = 0;      
    }
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

