#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    debug=true;
    
    kinect.init();
    kinect.setRegistration(true);
    kinect.open();
    kinect.setDepthClipping(500,2000);
    cout << kinect.getWidth() << "  " << kinect.getHeight() << "\n";
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
}

//--------------------------------------------------------------
void ofApp::update(){
    
    //imageBlur.setBlurOffset(200 * ofMap(mouseX, 0, ofGetWidth(), 0, 1, true));
    //imageBlur.setBlurPasses(10. * ofMap(mouseY, 0, ofGetHeight(), 1, 0, true));
    kinect.update();
    if (kinect.isFrameNew()) {
        
        trackerFace.update(kinect.getPixels());
        
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
    }
    else{
        //imageFbo.draw(0, ofGetHeight()/2, ofGetWidth()/2, ofGetHeight()/2);
        
                                        // image 1
        ofPushMatrix();
        ofTranslate(0, 0);
        ofScale(0.5, 0.5);
        kinect.draw(0, 0);
        for (auto instance : trackerFace.getInstances()) {
            instance.getLandmarks().getImageMesh().drawWireframe();
        }
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
