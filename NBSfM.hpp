#ifndef NBSfM_hpp
#define NBSfM_hpp

#include <iomanip>
#include <iostream>
#include "opencv2/opencv.hpp"
#include <unistd.h>

using namespace std;
using namespace cv;

#include <sys/types.h>
#include <dirent.h>
typedef std::vector<std::string> StringVec;

class NBSfM {
 private:
  // Parameters ====================
  // Image input
  string workspace_path_;
  string image_folder_path_;
  vector< string > image_paths_;
  int num_images_;

  // Video input
  string video_path_;
  int max_num_frames_;

  // Image/Video
  bool is_use_images_;
  bool is_use_video_;
  // Parameters ====================

  // Data ==========================
  // Images
  vector< Mat > images_;
  int image_width_;
  int image_height_;
  // Data ==========================

  // Parameter functions  ==========
  bool CheckParameters(int argc, char * argv[]);
  void ShowParameters();
  bool CheckWorkspace();
  bool CheckImageFolder();
  bool CheckImage(string image_name);
  bool CheckImagesInFolder();
  bool CheckVideo();
  void Help(int argc, char *argv[]);
  // Parameter functions  ==========

  // Helper functions ==============
  inline bool EndsWith(std::string const & value, std::string const & ending);
  void ReadDirectory(const std::string& name, StringVec& v);
  bool MakeDir(string path);
  // Helper functions ==============

  // 3D reconstruction functions ===
  bool ExportVideoFrames();
  bool LoadImages();
  // 3D reconstruction functions ===

 public:
  NBSfM(int argc, char * argv[]);
};
#endif /* NBSfM_hpp */
