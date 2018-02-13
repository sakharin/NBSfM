#ifndef NBSfM_hpp
#define NBSfM_hpp

#include <fstream>
#include <iomanip>
#include <iostream>
#include "opencv2/opencv.hpp"
#include <unistd.h>

using namespace std;
using namespace cv;

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
typedef std::vector<std::string> StringVec;

class NBSfM {
 private:
  // Parameters ====================
  // Image input
  string workspace_path_;
  string image_folder_path_;
  vector< string > image_names_;
  vector< string > image_paths_;
  int num_images_;

  // Video input
  string video_path_;
  int max_num_frames_;

  // Image/Video
  bool is_use_images_;
  bool is_use_video_;

  // Feature input
  string feature_folder_path_;
  string matched_feature_folder_path_;

  // Each step
  bool redo_feature_detection_;
  bool redo_feature_matching_;
  // Parameters ====================

  // Data ==========================
  // Images
  vector< Mat > images_;
  int image_width_;
  int image_height_;

  // Features
  vector< string > feature_paths_;
  Mat features_;
  int num_features_;

  // Matched features
  vector< string > matched_feature_paths_;
  Mat matched_features_;
  int num_matched_features_;
  // Data ==========================

  // Parameter functions  ==========
  bool CheckParameters(int argc, char * argv[]);
  void ShowParameters();
  bool CheckWorkspace();
  bool CheckImage(string image_name);
  bool CheckImagesInFolder();
  bool CheckFeatures();
  bool CheckMatchedFeatures();
  void Help(int argc, char *argv[]);
  // Parameter functions  ==========

  // Helper functions ==============
  inline bool EndsWith(std::string const & value, std::string const & ending);
  void ReadDirectory(const std::string& name, StringVec& v);
  bool MakeDir(string path);
  bool IsFolderExist(string path);
  bool IsFileReadable(string path);
  bool IsFileWritable(string path);
  vector< vector < string > > ReadCSV(string csv_path);
  bool CSVStr2Mat(vector< vector < string > > data, Mat& mat);
  string GetNameFromPath(string path);
  // Helper functions ==============

  // 3D reconstruction functions ===
  bool ExportVideoFrames();
  bool LoadImages();
  bool WriteReferenceImage();

  bool LoadFeatures();
  bool FeatureDetection();
  bool WriteFeatures();

  bool LoadMatchedFeatures();
  bool FeatureMatching();
  bool WriteMatchedFeatures();

  bool WriteFeatureImage();
  // 3D reconstruction functions ===

 public:
  NBSfM(int argc, char * argv[]);
};
#endif /* NBSfM_hpp */
