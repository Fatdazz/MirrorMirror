#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    

    debug=true;
    
    kinect.init();
    kinect.setRegistration(true);
    kinect.open();
    kinect.setDepthClipping(500,2000);
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
    imageBlur.setBlurOffset(0.5);
    imageBlur.setBlurPasses(2);
    
    imageBlur.setBlurOffset(7.0f);
    imageBlur.setBlurPasses(2);
    
    nearThreshold = 255;
    farThreshold = 236;
    //farThreshold =0;
    colormap.setMapFromName("pink");
}

//--------------------------------------------------------------
void ofApp::update(){
    
    //imageBlur.setBlurOffset(200 * ofMap(mouseX, 0, ofGetWidth(), 0, 1, true));
    //imageBlur.setBlurPasses(10. * ofMap(mouseY, 0, ofGetHeight(), 1, 0, true));
    kinect.update();
    if (kinect.isFrameNew()) {
        
        trackerFace.update(ofxCv::toCv(kinect.getPixels()));
        
        grayImage.setFromPixels(kinect.getDepthPixels());
        //kinect.getDepthPixels()
        
        
        ofPixels &pix = grayImage.getPixels();
        int numPixels = pix.size();
        for (int i=0; i<pix.size(); i++) {
            pix[i]=ofMap(max( (int)pix[i], (int) farThreshold), farThreshold, nearThreshold, 0, 255);
        }
        
        imageGray = grayImage.getPixels();
        colormap.apply(imageGray, imageColor);
    
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
        
        ofPushMatrix();
        ofSetLineWidth(1);
        //ofScale(1, 1);
        ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
        ofScale(5,5,5);
        ofDrawAxis(trackerFace.getScale());
        trackerFace.getObjectMesh().drawWireframe();
        ofPopMatrix();
    }
    else{
        //imageFbo.draw(0, ofGetHeight()/2, ofGetWidth()/2, ofGetHeight()/2);
        ofPushMatrix();
        //ofScale(0.5, 0.5);
        ofTranslate(0, 0);
        //kinect.draw(0, 0, ofGetWidth(), ofGetHeight());
        kinect.draw(0, 0);
        ofPopMatrix();
        
        ofPushMatrix();
        //ofTranslate(ofGetWidth()/kinect.width, ofGetHeight()/kinect.height);
        ofSetLineWidth(1);
        
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::LEFT_EYE).draw();
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::RIGHT_EYE).draw();
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::LEFT_EYEBROW).draw();
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::RIGHT_EYEBROW).draw();
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::NOSE_BRIDGE).draw();
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::NOSE_BASE).draw();
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::INNER_MOUTH).draw();
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::OUTER_MOUTH).draw();
        trackerFace.getImageFeature(ofxFaceTrackerThreaded::JAW).draw();
        
        ofTranslate(trackerFace.getPosition());
        //ofxCv::applyMatrix(trackerFace.getRotationMatrix());
        
        trackerFace.getObjectMesh().drawWireframe();
        ofPopMatrix();
        
        grayImage.draw(ofGetWidth()/2, 0, ofGetWidth()/2, ofGetHeight()/2);
        
        ofPushMatrix();
        ofScale(0.5, 0.5);
        ofTranslate(0, ofGetHeight());
        imageBlur.drawBlurFbo();
        ofPopMatrix();
    }

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    if (key==' ') debug=!debug;
    
    if (debug) {
        switch (key) {
            case 'r':
                cout << "BlurOffse: " << imageBlur.getBlurOffset() << " BlurPasses: " <<imageBlur.getBlurPasses() << endl;
                break;
            case 'o':
                farThreshold++;
                cout << "far: " << farThreshold <<" near: "<<nearThreshold << endl;
                break;
            case 'p':
                farThreshold--;
                cout << "far: " << farThreshold <<" near: "<<nearThreshold << endl;
                break;
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
