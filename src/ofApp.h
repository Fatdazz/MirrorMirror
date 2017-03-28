#pragma once

#include "ofMain.h"
#include "ofxKinect.h"
#include "ofxOpenCv.h"
#include "ofxColorMap.h"
#include "ofxMultiFboBlur.h"
#include "ofxFaceTrackerThreaded.h"
#include "ofxFaceTracker2.h"
#include "ofxCv.h"
#include "FaceTrackerThreaded.h"

class faceAnimation {
    public:
    ofSoundBuffer soundBuffer;
    vector<ofPolyline>  face;
    
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
        void audioIn(ofSoundBuffer& buffer);
        void audioOut(ofSoundBuffer &outBuffer);
        void exit(){
            trackerFace.stopThread();
        }
        void tranposeRotation(ofMatrix4x4 *_Matrix);
        vector<int> consecutive(int start, int end);

    
    bool debug;
    ofxKinect kinect;
    int nearThreshold;
    int farThreshold;
    
    ofxCvGrayscaleImage grayImage;
    ofxCvGrayscaleImage grayThreshNear;
    ofxCvGrayscaleImage grayThreshFar;
    ofxCvGrayscaleImage grayImageMap;
    
    ofxColorMap         colormap;
    ofImage             imageColor,imageGray;
    ofFbo               imageFbo;
    ofxMultiFboBlur     imageBlur;
    
    ofxCv::ContourFinder   contourFinder;
    FaceTrackerThreaded trackerFace;
    
    vector<faceAnimation>  faceAnimationVect;
    faceAnimation          *faceAnimationPtr;
    ofPolyline             Animation, bufferAnimation;
    bool                   rec,play;
    ofSoundStream          soundStream;
    int                    bufferCounter;
		
};
