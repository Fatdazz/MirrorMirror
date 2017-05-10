#include "ofApp.h"

#define STRINGIFY(x) #x

static string depthFragmentShader =
  STRINGIFY(
	    uniform sampler2DRect tex;
	    void main()
	    {
              vec4 col = texture2DRect(tex, gl_TexCoord[0].xy);
              float value = col.r;
              float low1 = 500.0;
              float high1 = 750.0;
              float low2 = 1.0;
              float high2 = 0.0;
              float d = clamp(low2 + (value - low1) * (high2 - low2) / (high1 - low1), 0.0, 1.0);
              if (d == 1.0) {
		d = 0.0;
              }
              gl_FragColor = vec4(vec3(d), 1.0);
	    }
	    );

static string irFragmentShader =
  STRINGIFY(
	    uniform sampler2DRect tex;
	    void main()
	    {
              vec4 col = texture2DRect(tex, gl_TexCoord[0].xy);
              float value = col.r / 65535.0;
              gl_FragColor = vec4(vec3(value), 1.0);
	    }
	    );



#include "GpuRegistration.h"



//--------------------------------------------------------------
void ofApp::setup(){

#if USE_KINECT_2
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
#else
  kinect.init();
  kinect.setRegistration(true);
  kinect.open();
  kinect.setDepthClipping(500,2000);
#endif
    
  // init Face traker
  trackerFace.setup();
    
  // init contourFinder
  contourFinder.setMinAreaRadius(90); // � determin� <===============================
  contourFinder.setMaxAreaRadius(300);
  contourFinder.setUseTargetColor(false);
  contourFinder.setThreshold(0);

#if USE_KINECT_2
  grayImage.allocate(512, 424);
  grayThreshFar.allocate(512, 424);
  grayThreshNear.allocate(512, 424);
  grayImageMap.allocate(512, 424);
  imageGray.allocate(512, 424, ofImageType::OF_IMAGE_GRAYSCALE);
  imageColor.allocate(1920, 1080, ofImageType::OF_IMAGE_COLOR);
  //imageFbo.allocate(512, 424);
#else    
  grayImage.allocate(kinect.width, kinect.height);
  grayThreshFar.allocate(kinect.width, kinect.height);
  grayThreshNear.allocate(kinect.width, kinect.height);
  grayImageMap.allocate(kinect.width, kinect.height);
  imageGray.allocate(kinect.width, kinect.height, ofImageType::OF_IMAGE_GRAYSCALE);
  imageColor.allocate(kinect.width, kinect.height, ofImageType::OF_IMAGE_COLOR);
  //imageFbo.allocate(kinect.width, kinect.height);
#endif
    
  ofFbo::Settings fboS;
  fboS.width = ofGetWidth();
  fboS.height = ofGetHeight();
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
  colormap.setMapFromName("pink");
    
  int bufferSize = 256;
  soundStream.setup(this, 2, 1, 48000, bufferSize, 4);
  rec = play = false;
  bufferCounter = 0;
  faceAnimationPtr = NULL;
}

//--------------------------------------------------------------
void ofApp::update(){
    
  //imageBlur.setBlurOffset(200 * ofMap(mouseX, 0, ofGetWidth(), 0, 1, true));
  //imageBlur.setBlurPasses(10. * ofMap(mouseY, 0, ofGetHeight(), 1, 0, true));
  kinect.update();
  if (kinect.isFrameNew()) {

#if USE_KINECT_2

    tmp.setFromPixels(kinect.getColorPixelsRef());
    //tmp.resize(512, 424);
    //tmp.update();
    //trackerFace.update(ofxCv::toCv(tmp));
    grayImage.setFromPixels(kinect.getDepthPixelsRef());

    colorTex0.loadData(kinect.getColorPixelsRef());
    depthTex0.loadData(kinect.getDepthPixelsRef());
    //irTex0.loadData(kinect.getIrPixelsRef());

    gr.update(depthTex0, colorTex0, true);

    ofPixels pp;
    gr.getRegisteredTexture().readToPixels(pp);

    img.setFromPixels(pp);

    //std::cout << "img: " << img.getWidth() << " " << img.getHeight() << "\n";
    trackerFace.update(ofxCv::toCv(img));

    depthTex0.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    //gr.update(depthTex0, colorTex0, true);

    ofPixels p;
    depthTex0.readToPixels(p);
    grayImage.setFromPixels(p);

#else
    trackerFace.update(ofxCv::toCv(kinect.getPixels()));
    grayImage.setFromPixels(kinect.getDepthPixels());
#endif
    /*
      ofPixels &pix = grayImage.getPixels();
      int numPixels = pix.size();
        
      for (int i=0; i<pix.size(); i++) {
      pix[i] = ofMap(max( (int)pix[i], (int) farThreshold), farThreshold, nearThreshold, 0, 255);
      }

    */

    imageGray = grayImage.getPixels();
           contourFinder.findContours(imageGray);
        
    if (contourFinder.getPolylines().size() > 0 && !trackerFace.isThreadRunning()) {
      trackerFace.startThread();
      cout << "start" << endl;

    }
    //std::cout << std::boolalpha << trackerFace.getFound() << "\n";

    if (contourFinder.getPolylines().size() == 0 && trackerFace.isThreadRunning()) {
      //trackerFace.stopThread();
      cout << "stop" << endl;
    }

    imageGray.resize(1920, 1080);
    colormap.apply(imageGray, imageColor);

    // Buffer Face
    bufferAnimation = trackerFace.getObjectMesh();
  }
}
//--------------------------------------------------------------
void ofApp::draw(){
  ofSetWindowTitle(ofToString(ofGetFrameRate()));
     
  ofMesh face = trackerFace.getImageMesh();
  for (int i=0; i<face.getVertices().size(); i++) {
    face.addTexCoord(face.getVertices()[i]);
  }

    
  if (!debug) {
    imageBlur.drawBlurFbo();
  } else {
    //imageFbo.draw(0, ofGetHeight()/2, ofGetWidth()/2, ofGetHeight()/2);
    ofSetColor(ofColor::white);
        
    // image 1
    ofPushMatrix();
    ofTranslate(0, 0);
    //ofScale(0.5, 0.5);
    if (trucAlexTropCool) {
      ofPixels pix;
      depthTex0.readToPixels(pix);
      ofImage img;
      img.setFromPixels(kinect.getDepthPixelsRefAlex());
      //img.resize(1024, 848);

      ofTexture tex;
      tex.loadData(img.getPixels());
      depthShader.begin();
      depthTex0.draw(0,0, 1024, 848);
      depthShader.end();

    } else {
      img.draw(0, 0);
    }

#if USE_KINECT_2
    //ofImage img;
    //img.setFromPixels(kinect.getColorPixelsRef());
    //img.draw(0, 0);
#else
    kinect.draw(0,0);
#endif
    ofSetColor(ofColor::blue);
    imageColor.bind();
    trackerFace.getImageMesh().drawWireframe();
    imageColor.unbind();
    ofSetColor(ofColor::white);
    ofPopMatrix();

    // image 2
    /*
      ofPushMatrix();
      ofTranslate(0, 0);
      ofScale(0.5, 0.5);
      grayImage.draw(1000, 0);
      ofPopMatrix();
    */
        
    // image 3
    /*
      ofPushMatrix();
      ofTranslate(0, grayImage.getHeight());
      ofScale(0.5, 0.5);
      imageBlur.drawBlurFbo();
      ofPopMatrix();
    */

    if (faceAnimationPtr != NULL && play) {

      ofPushMatrix();
      unique_lock<mutex> lock(audioMutex);
      if (trackerFace.getFound()) {
	ofMesh lulu = faceAnimationPtr->face[bufferCounter];
	for (int i=0; i<lulu.getVertices().size(); i++) {
	  ofVec3f point = lulu.getVertices()[i];
	  ofMatrix4x4 transformation;
	  float scale = trackerFace.getScale();
	  transformation.makeScaleMatrix(ofVec3f(scale,scale,0));
	  transformation = transformation * trackerFace.getRotationMatrix();
	  point = point * transformation;
	  point = ofVec3f(point.x + trackerFace.getPosition().x, point.y + trackerFace.getPosition().y, 0);
	  lulu.setVertex(i, point);
	}

	faceAnimationPtr->face[bufferCounter] = lulu;

	ofMesh face = trackerFace.getImageMesh();
	for (int i=0; i<face.getVertices().size(); i++) {
	  face.addTexCoord(face.getVertices()[i]);
	}

	for (int i = 48; i < 66; i++) {
	  face.setVertex(i, faceAnimationPtr->face[bufferCounter].getVertices()[i]);
	}
      }

	imageBlur.beginDrawScene();
	ofClear(0,0,0);
	ofSetColor(ofColor::white);
	if (trucAlexTropCool) {
	imageColor.draw(0,0);
	imageColor.bind();
	face.draw();
	imageColor.unbind();
	} else {
	  imageGray.resize(1024, 848);
	  imageGray.draw(0,0);
	  imageGray.bind();
	  face.draw();
	  imageGray.unbind();
	}
	imageBlur.endDrawScene();
	imageBlur.performBlur();

	imageBlur.drawBlurFbo();

	ofPopMatrix();
    }
    ofPopMatrix();
  }
  //gr.getRegisteredTexture().draw(0, 0);

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
  case 'w':
    trucAlexTropCool = !trucAlexTropCool;
    break;
  }
}
//--------------------------------------------------------------
void ofApp::audioIn( ofSoundBuffer& buffer ){
  /*
    #if USE_KINECT_2
    // L'addon kinect 2 n'a pas de fonction pour si elle est connecté
    // on se repere avec l'arrivee de nouvelles frames
    if (rec && faceAnimationPtr != NULL && kinect.isFrameNew()) {
    #else
    if (rec && faceAnimationPtr != NULL && kinect.isConnected()) {
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
  if (play && faceAnimationPtr != NULL && bufferCounter < (faceAnimationPtr->soundBuffer.size()/soundStream.getBufferSize())-1) {
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

vector<int> ofApp::consecutive(int start, int end) {
  int n = end - start;
  vector<int> result(n);
  for(int i = 0; i < n; i++) {
    result[i] = start + i;
  }
  return result;
}
