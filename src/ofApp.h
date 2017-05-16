#pragma once

#include "ofMain.h"
#define USE_KINECT_2 1

#if USE_KINECT_2
#include "ofxKinectV2.h"
#else
#include "ofxKinect.h"
#endif
#include "ofxOpenCv.h"
#include "ofxColorMap.h"
#include "ofxMultiFboBlur.h"
#include "ofxFaceTrackerThreaded.h"
#include "ofxCv.h"
#include "FaceTrackerThreaded.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <fstream>
#include "ofxFilterLibrary.h"



constexpr int win_width = 1920;
constexpr int win_height = 1080;
constexpr int width_kinect = 1920;
constexpr int height_kinect = 1080;

class ofSoundBufferCereal : public ofSoundBuffer {
public:
  friend class cereal::access;

  template<typename Archive>
    void serialize(Archive& archive) {
    archive( buffer, channels, samplerate, tickCount, soundStreamDeviceID);
  }
};


class FaceAnimation {
 public:
  ofSoundBufferCereal soundBuffer;
  vector<ofMesh>  face;
};

class ofApp : public ofBaseApp{

 public:


	ofApp() : filter(win_width, win_height) {
	}

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
  void exit() {
    trackerFace.waitForThread();
    kinect.close();
  }
  void tranposeRotation(ofMatrix4x4 *_Matrix);
    
  bool debug = true;


#if USE_KINECT_2
  ofxKinectV2 kinect;
  SobelEdgeDetectionFilter filter;

#else
  ofxKinect kinect;
#endif
    
  int nearThreshold;
  int farThreshold;

  ofxColorMap         colormap;
  ofImage             imageColor,imageGray;
  ofxMultiFboBlur     imageBlur;
  
  FaceTrackerThreaded trackerFace;
    
  vector<FaceAnimation>  faceAnimationVect;
  FaceAnimation          *faceAnimationPtr;
  ofMesh             animation, bufferAnimation, animationMouth;
  bool                   rec,play;

  /*
   *
   * ATTENTION: il ne faut pas que d'autres logiciels utilisent du son
   * genre firefox etc
   * sinon ca casse tout
   * ou alors faut faire des reglages chelou avec alsa ou jack, et flemme
   *
   */
  ofSoundStream          soundStream;
  int                    bufferCounter;

  mutex audioMutex;

  ofImage imgGr;
  ofPixels tmpPixels;

  ofFbo fboGray;
  ofImage grayImage;

  ofFbo fboColorMaskAndBackground;

  int numFiles;

  ofFbo fbo;

};
