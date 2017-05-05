#include "ofApp.h"
//--------------------------------------------------------------
void ofApp::setup(){
    
    debug=true;
    // init kinect
    #if USE_KINECT_2
    kinect.open();
    kinect.update();
    kinectTex.loadData(kinect.getDepthPixels());
    #else
    kinect.init();
    kinect.setRegistration(true);
    kinect.open();
    kinect.setDepthClipping(500,2000);
    #endif
    
    // init Face traker
    trackerFace.setup();
    
    // init contourFinder
    contourFinder.setMinAreaRadius(90); // à determiné <===============================
    contourFinder.setMaxAreaRadius(300);
    contourFinder.setUseTargetColor(false);
    contourFinder.setThreshold(0);

#if USE_KINECT_2
    grayImage.allocate(1080, 720);
    grayThreshFar.allocate(1080, 720);
    grayThreshNear.allocate(1080, 720);
    grayImageMap.allocate(1080, 720);
    imageGray.allocate(1080, 720, ofImageType::OF_IMAGE_GRAYSCALE);
    imageColor.allocate(1080, 720, ofImageType::OF_IMAGE_COLOR);
    imageFbo.allocate(1080, 720);
#else    
    grayImage.allocate(kinect.width, kinect.height);
    grayThreshFar.allocate(kinect.width, kinect.height);
    grayThreshNear.allocate(kinect.width, kinect.height);
    grayImageMap.allocate(kinect.width, kinect.height);
    imageGray.allocate(kinect.width, kinect.height, ofImageType::OF_IMAGE_GRAYSCALE);
    imageColor.allocate(kinect.width, kinect.height, ofImageType::OF_IMAGE_COLOR);
    imageFbo.allocate(kinect.width, kinect.height);
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
      ofImage tmp;
      tmp.setFromPixels(kinect.getRgbPixels());
      trackerFace.update(ofxCv::toCv(tmp));        
      grayImage.setFromPixels(kinect.getDepthPixels());
#else
      trackerFace.update(ofxCv::toCv(kinect.getPixels()));        
      grayImage.setFromPixels(kinect.getDepthPixels());
#endif
        
        ofPixels &pix = grayImage.getPixels();
        int numPixels = pix.size();
        
        for (int i=0; i<pix.size(); i++) {
            pix[i]=ofMap(max( (int)pix[i], (int) farThreshold), farThreshold, nearThreshold, 0, 255);
        }
        imageGray = grayImage.getPixels();
	imageGray.resize(1080, 720);
	imageGray.update();
        
        contourFinder.findContours(imageGray);
        
        if (contourFinder.getPolylines().size() > 0 && !trackerFace.isThreadRunning()) {
            trackerFace.startThread();
            cout << "start" << endl;
        }
        if (contourFinder.getPolylines().size() == 0 && trackerFace.isThreadRunning()) {
	  // trackerFace.stopThread(); << remettre ca
            cout << "stop" << endl;
        }
        
        colormap.apply(imageGray, imageColor);
        
        // Buffer Face
        if (trackerFace.getFound()) {
            //ofMatrix4x4 matrix = trackerFace.getRotationMatrix();
            //tranposeRotation(&matrix);
            bufferAnimation.clear();
            bufferAnimation = trackerFace.getObjectFeature(ofxFaceTracker::ALL_FEATURES);
        }
    }
}
//--------------------------------------------------------------
void ofApp::draw(){
    ofSetWindowTitle(ofToString(ofGetFrameRate()));
     
    ofMesh face = trackerFace.getImageMesh();
    for (int i=0; i<face.getVertices().size(); i++) {
        face.addTexCoord(face.getVertices()[i]);
    }
    
    if (play) {
        vector<int> vect = consecutive(1,68);
	cout << "Buffer counter: " << bufferCounter << "\n";
	cout << "face size: " << face.getVertices().size() << "\n";
        for (int i = 0; face.getVertices().size(); i++) {
	  face.setVertex(i, faceAnimationPtr->face[bufferCounter].getVertices()[i] * trackerFace.getRotationMatrix());
        }
    }
    imageBlur.beginDrawScene();
    ofClear(0,0,0);
    ofSetColor(ofColor::white);
    imageColor.draw(0,0, ofGetWidth(), ofGetHeight());
    imageColor.bind();
    face.draw();
    imageColor.unbind();
    imageBlur.endDrawScene();
    imageBlur.performBlur();
    
    if (!debug) {
        imageBlur.drawBlurFbo();
    }
    else{
        //imageFbo.draw(0, ofGetHeight()/2, ofGetWidth()/2, ofGetHeight()/2);
        ofSetColor(ofColor::white);
        
                                        // image 1
        ofPushMatrix();
        ofTranslate(0, 0);
        ofScale(0.5, 0.5);
#if USE_KINECT_2
	ofImage tmp;
	tmp.setFromPixels(kinect.getRgbPixels());
	tmp.draw(0, 0);
#else
	kinect.draw(0,0);
#endif
        trackerFace.getImageMesh().drawWireframe();
        ofPopMatrix();
                                        // image 2
        ofPushMatrix();
        ofTranslate(ofGetWidth()/2, 0);
        ofScale(0.5, 0.5);
        grayImage.draw(0, 0);
        ofPopMatrix();
        
                                        // image 3
        ofPushMatrix();
        ofTranslate(0, ofGetHeight()/2);
        ofScale(0.5, 0.5);
        imageBlur.drawBlurFbo();
        ofPopMatrix();
                                        // image 4
        ofPushMatrix();
        ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
        ofScale(0.5, 0.5);
        
        
        if (trackerFace.getFound()) {
            //trackerFace.getImageFeature(ofxFaceTracker::ALL_FEATURES).draw();
            ofMesh lulu = trackerFace.getObjectMesh();
            //ofMatrix4x4 tranformation = trackerFace.getRotationMatrix();
            //tranformation.setTranslation(trackerFace.getPosition().x,trackerFace.getPosition().x,0);
            //tranformation.translate(trackerFace.getPosition().x,trackerFace.getPosition().y,0);
            
            for (int i=0; i<lulu.getVertices().size(); i++) {
                ofVec3f point = lulu.getVertices()[i];
                ofMatrix4x4 transformation;
                float scale = trackerFace.getScale();
                transformation.makeScaleMatrix(ofVec3f(scale,scale,scale));
                transformation = transformation * trackerFace.getRotationMatrix();
                //transformation._mat[3][0] = trackerFace.getPosition().x;
                //transformation._mat[3][0] = trackerFace.getPosition().y;
                point = point * transformation;
                point = ofVec3f(point.x + trackerFace.getPosition().x,point.y + trackerFace.getPosition().y,0);
                lulu.setVertex(i, point);
            }
            lulu.drawWireframe();
            ofSetColor(ofColor::red);
            vector<int> vect = consecutive(48,66);
            for (int i=0; i<vect.size(); i++) {
                ofDrawCircle(face.getVertices()[vect[i]].x, face.getVertices()[vect[i]].y, 4);
            }
            ofSetColor(ofColor::white);
            
            /*
            ofPushMatrix();
            ofTranslate(ofGetWindowWidth()/2, ofGetHeight()/2);
            ofScale(trackerFace.getScale()+3,trackerFace.getScale()+3,0);
            trackerFace.getObjectFeature(ofxFaceTracker::ALL_FEATURES).draw();
            trackerFace.getObjectMesh().drawWireframe();
            ofPopMatrix();
            */
        }
        if (faceAnimationPtr != NULL && play) {
            ofPushMatrix();
            ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
            ofScale(5, 5);
            ofSetColor(ofColor::red);
            faceAnimationPtr->face[bufferCounter].draw();
            ofPopMatrix();
        }
        ofPopMatrix();
    }

}
//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == ' ') debug=!debug;
    if (debug) {
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
                if (rec == false && trackerFace.getFound()) {
                    faceAnimation newAnim;
                    faceAnimationVect.push_back(newAnim);
                    faceAnimationPtr = &faceAnimationVect.at(faceAnimationVect.size()-1);
                }
                rec = !rec;
                break;
            case 'p':
                cout << " Play " << endl;
                bufferCounter = 0;
                play=!play;
                break;
        }
    }
}
//--------------------------------------------------------------
void ofApp::audioIn( ofSoundBuffer& buffer ){

#if USE_KINECT_2
  // L'addon kinect 2 n'a pas de fonction pour si elle est connect√©
  // on se repere avec l'arrivee de nouvelles frames
  if (rec && faceAnimationPtr != NULL && kinect.isFrameNew()) {
#else
    if (rec && faceAnimationPtr != NULL && kinect.isConnected()) {
#endif
        // Enregistrement Audio
        faceAnimationPtr->soundBuffer.append(buffer);
        
        // Enregistre les 68 pt du face tracking
        faceAnimationPtr->face.push_back(bufferAnimation);
    }
}
//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer &outBuffer){
    auto nChannel = outBuffer.getNumChannels();
    if (play && faceAnimationPtr != NULL && bufferCounter < (faceAnimationPtr->soundBuffer.size()/(soundStream.getBufferSize())-1)) {
        for (int i = 0; i < outBuffer.getNumFrames(); i++) {
            // Lecture Audio
            outBuffer.getBuffer()[i*nChannel] = outBuffer.getBuffer()[i*nChannel + 1] = faceAnimationPtr->soundBuffer.getBuffer()[i + bufferCounter * outBuffer.getNumFrames()];
        }
        bufferCounter++;
    }
    else{
        //bufferCounter = 0;
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
