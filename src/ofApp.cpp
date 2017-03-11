#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    debug=true;
    // init kinect
    kinect.init();
    kinect.setRegistration(true);
    kinect.open();
    kinect.setDepthClipping(500,2000);
    cout << kinect.getWidth() << "  " << kinect.getHeight() << "\n";
    
    // init Face traker
    trackerFace.setup();
    
    grayImage.allocate(kinect.width, kinect.height);
    grayThreshFar.allocate(kinect.width, kinect.height);
    grayThreshNear.allocate(kinect.width, kinect.height);
    grayImageMap.allocate(kinect.width, kinect.height);
    imageGray.allocate(kinect.width, kinect.height, ofImageType::OF_IMAGE_GRAYSCALE);
    imageColor.allocate(kinect.width, kinect.height, ofImageType::OF_IMAGE_COLOR);
    imageFbo.allocate(kinect.width, kinect.height);
    
    
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
    soundStream.setup(this, 2, 1, 44100, bufferSize, 4);
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
        
        trackerFace.update(kinect.getPixels());
        
        grayImage.setFromPixels(kinect.getDepthPixels());
        

        ofPixels &pix = grayImage.getPixels();
        int numPixels = pix.size();
        for (int i=0; i<pix.size(); i++) {
            pix[i]=ofMap(max( (int)pix[i], (int) farThreshold), farThreshold, nearThreshold, 0, 255);
        }
        
        imageGray = grayImage.getPixels();
        colormap.apply(imageGray, imageColor);
        
        // Buffer Face
        if (trackerFace.getInstances().size() > 0) {
            /*
            ofMatrix4x4 MatrixInv =  trackerFace.getInstances()[0].getPoseMatrix();
            MatrixInv.scale(-1, 1, 1);
            tranposeRotation(&MatrixInv);
            */
            ofMatrix4x4 matrix = trackerFace.getInstances()[0].getPoseMatrix();
            ofMatrix4x4 matrixInv;
            matrixInv.setRotate(matrix.getRotate().inverse());
            matrixInv.setTranslation(matrix.getTranslation());
            matrixInv.scale(matrix.getScale());
            ofPolyline polyFace = trackerFace.getInstances()[0].getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::ALL_FEATURES);
            //MatrixInv.scale(10, 10, 0);
            ofPolyline temp;
            bufferAnimation.clear();
            for (int i=0; i<polyFace.size(); i++) {
                temp.addVertex(polyFace[i] * matrixInv);
                //temp.addVertex(polyFace[i]);

            }
            bufferAnimation = temp;
        }
    
    }

}

//--------------------------------------------------------------
void ofApp::draw(){

    imageBlur.beginDrawScene();
    ofClear(0,0,0);
    ofSetColor(ofColor::white);
    imageColor.draw(0,0, ofGetWidth(), ofGetHeight());
    imageBlur.endDrawScene();
    imageBlur.performBlur();
    
    if (!debug) {
        imageBlur.drawBlurFbo();
    }
    else{
        //imageFbo.draw(0, ofGetHeight()/2, ofGetWidth()/2, ofGetHeight()/2);
        
                                        // image 1
        ofPushMatrix();
        ofTranslate(0, 0);
        ofScale(0.5, 0.5);
        kinect.draw(0, 0);

        
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
        ofTranslate(ofGetWindowWidth()/2, ofGetHeight()/2);
        ofScale(0.5, 0.5);
        
        for (auto instance : trackerFace.getInstances()) {
            instance.getLandmarks().getImageFeature(ofxFaceTracker2Landmarks::ALL_FEATURES).draw();
        }
        if (faceAnimationPtr != NULL && play) {
            
            faceAnimationPtr->face[bufferCounter].draw();

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
                if (rec == false && trackerFace.getInstances().size()!=0) {
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
    
    if (rec && faceAnimationPtr != NULL && kinect.isConnected()) {
        // Enregistrement Audio
        faceAnimationPtr->soundBuffer.append(buffer);
        
        // Enregistre les 68 pt du face tracking
        faceAnimationPtr->face.push_back(bufferAnimation);
    }
}
//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer &outBuffer){
    auto nChannel = outBuffer.getNumChannels();
    if (play && faceAnimationPtr != NULL && bufferCounter < faceAnimationPtr->soundBuffer.size()/(soundStream.getBufferSize())-1) {
        
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
    
    _Matrix->_mat[0][1]+= _Matrix->_mat[1][0]; _Matrix->_mat[1][0] = _Matrix->_mat[0][1] -_Matrix->_mat[1][0]; _Matrix->_mat[0][1]-= _Matrix->_mat[1][0];
    
    _Matrix->_mat[0][2]+= _Matrix->_mat[2][0]; _Matrix->_mat[2][0] = _Matrix->_mat[0][2] -_Matrix->_mat[2][0]; _Matrix->_mat[0][2]-= _Matrix->_mat[2][0];
    
    _Matrix->_mat[2][1]+= _Matrix->_mat[1][2]; _Matrix->_mat[1][2] = _Matrix->_mat[2][1] -_Matrix->_mat[1][2]; _Matrix->_mat[2][1]-= _Matrix->_mat[1][2];
    
    _Matrix->_mat[2][0]= -_Matrix->_mat[2][0]; _Matrix->_mat[2][1]= -_Matrix->_mat[2][1]; _Matrix->_mat[2][2]= -_Matrix->_mat[2][2];
    
}
