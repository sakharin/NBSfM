#include "NBSfM.hpp"

NBSfM::NBSfM(int argc, char *argv[]) :
    workspace_path_("."),
    image_folder_path_("./images"),
    video_path_(""),
    max_num_frames_(30),

    is_use_images_(false),
    is_use_video_(false),

    feature_folder_path_("./features"),

    redo_feature_detection_(false),
    num_features_(0) {
  if (!CheckParameters(argc, argv)) {
    exit(-1);
  }

  ShowParameters();

  if (is_use_video_) {
    ExportVideoFrames();
  }

  LoadImages();
  WriteReferenceImage();

  if (redo_feature_detection_) {
    FeatureDetection();
    WriteFeatures();
  } else {
    LoadFeatures();
  }
  WriteFeatureImage();
}

bool NBSfM::CheckParameters(int argc, char * argv[]) {
  int index = 1;
  while (index < argc) {
    if (index + 1 < argc && strcmp(argv[index], "--workspace_path") == 0) {
      workspace_path_.assign(argv[index + 1]);
      if (!CheckWorkspace()) {
        return false;
      }
      index += 2;
    } else if (index + 1 < argc && strcmp(argv[index], "--image_paths") == 0) {
      num_images_ = 0;
      image_paths_.clear();

      index += 1;
      while (index < argc && string(argv[index]).compare(0, 2, "--") != 0) {
        if (access(argv[index], F_OK) != 0) {
          cerr << "Cannot read an image : " << argv[index] << "." << endl;
          return false;
        }
        image_paths_.push_back(argv[index]);
        if (!CheckImage(image_paths_.back())) {
          cerr << "Cannot read an image : " << argv[index] << "." << endl;
          return false;
        }
        num_images_++;
        index += 1;
      }
      is_use_video_ = false;
      is_use_images_ = true;
    } else if (index + 1 < argc && strcmp(argv[index], "--image_folder") == 0) {
      image_folder_path_.assign(argv[index + 1]);
      if (!CheckImageFolder()) {
        cerr << "Cannot find folder : " << image_folder_path_ << "." << endl;
        return false;
      } else {
        CheckImagesInFolder();
      }
      is_use_video_ = false;
      is_use_images_ = true;
      index += 2;
    } else if (index + 3 < argc && strcmp(argv[index], "--video") == 0 &&
               string(argv[index + 1]).compare(0, 2, "--") != 0 &&
               string(argv[index + 2]).compare(0, 2, "--") != 0 &&
               string(argv[index + 3]).compare(0, 2, "--") != 0) {
      video_path_.assign(argv[index + 1]);
      max_num_frames_ = strtod(argv[index + 2], NULL);
      image_folder_path_.assign(argv[index + 3]);
      if (max_num_frames_ < 2) {
        cerr << "max_num_frames must be greater than 2." << endl;
        return false;
      }
      CheckVideo();
      is_use_images_ = false;
      is_use_video_ = true;
      index += 4;
    } else if (index + 2 < argc && strcmp(argv[index], "--video") == 0 &&
               string(argv[index + 1]).compare(0, 2, "--") != 0 &&
               string(argv[index + 2]).compare(0, 2, "--") != 0) {
      video_path_.assign(argv[index + 1]);
      max_num_frames_ = strtod(argv[index + 2], NULL);
      image_folder_path_.assign(workspace_path_ + "/images");
      if (max_num_frames_ < 2) {
        cerr << "max_num_frames must be greater than 2." << endl;
        return false;
      }
      CheckVideo();
      is_use_images_ = false;
      is_use_video_ = true;
      index += 3;
    } else if (index + 1 < argc && strcmp(argv[index], "--feature_folder") == 0) {
      feature_folder_path_.assign(argv[index + 1]);
      if (!CheckFeature()) {
        return false;
      }
      index += 2;
    } else {
      Help(argc, argv);
      return false;
    }
  }

  if (access(workspace_path_.c_str(), W_OK) != 0) {
    cerr << "Cannot write on workspace_path : " << workspace_path_ << "." << endl;
    return false;
  }
  if (is_use_images_) {
    if (image_paths_.size() < 2) {
        cerr << "At least 2 images are required." << endl;
        return false;
    }
  } else if (is_use_video_) {
    if (access(video_path_.c_str(), F_OK) != 0) {
      cerr << "Cannot find video_path : " << video_path_ << "." << endl;
      return false;
    }
    if (access(image_folder_path_.c_str(), F_OK | W_OK) != 0) {
      cerr << "Cannot write on image_folder_path : " << image_folder_path_ << "." << endl;
      return false;
    }
  } else {
    cerr << "A video or 2 images are required." << endl;
    return false;
  }
  return true;
}

void NBSfM::ShowParameters() {
  cout << "                      Parameters" << endl;
  cout << "                         Input" << endl;
  cout << "            workspace_path : " << workspace_path_ << endl;

  if (is_use_images_) {
    vector< string >::iterator it = image_paths_.begin();
  cout << "                num_images : " << num_images_ << endl;
  cout << "               image_paths : " << *it << endl;
  ++it;
    for (; it < image_paths_.end(); ++it) {
  cout << "                             " << *it << endl;
    }
  } else if (is_use_video_) {
  cout << "                video_path : " << video_path_ << endl;
  cout << "         image_folder_path : " << image_folder_path_ << endl;
  cout << "            max_num_frames : " << max_num_frames_ << endl;
  }
  cout << "            feature_folder : " << feature_folder_path_ << endl;
  cout << endl;
  cout << "                 Recalculate each step" << endl;
  cout << "    redo_feature_detection : " << ((redo_feature_detection_) ? "true" : "false") << endl;
}

bool NBSfM::CheckWorkspace() {
  // Check images
  image_folder_path_.assign(workspace_path_ + "/images");
  if (CheckImageFolder()) {
    CheckImagesInFolder();
    is_use_video_ = false;
    is_use_images_ = true;
  } else {
    num_images_ = 0;
    image_paths_.clear();
  }

  // Check features
  feature_folder_path_.assign(workspace_path_ + "/features");
  CheckFeature();
  return true;
}

bool NBSfM::CheckImageFolder() {
  if (access(image_folder_path_.c_str(), F_OK | R_OK) == 0) {
    return true;
  }
  return false;
}

bool NBSfM::CheckImage(string image_name) {
  return (EndsWith(image_name, ".JPG") ||
          EndsWith(image_name, ".JPEG") ||
          EndsWith(image_name, ".jpeg") ||
          EndsWith(image_name, ".jpg") ||
          EndsWith(image_name, ".PNG") ||
          EndsWith(image_name, ".png"));
}

bool NBSfM::CheckImagesInFolder() {
  num_images_ = 0;
  image_paths_.clear();

  StringVec str_vec;
  ReadDirectory(image_folder_path_, str_vec);
  sort(str_vec.begin(), str_vec.end());
  vector< string > tmp_image_paths;
  for (unsigned int i = 0; i < str_vec.size(); i++) {
    if (CheckImage(str_vec[i])) {
      tmp_image_paths.push_back(image_folder_path_ + "/" + str_vec[i]);
    }
  }
  if (tmp_image_paths.size() >= 2) {
    image_paths_.clear();
    for (unsigned int i = 0; i < tmp_image_paths.size(); i++) {
      image_paths_.push_back(tmp_image_paths[i]);
    }
    num_images_ = image_paths_.size();
  } else {
    return false;
  }
  return true;
}

bool NBSfM::CheckVideo() {
  if (access(video_path_.c_str(), F_OK | R_OK) != 0) {
    return false;
  }
  if (access(image_folder_path_.c_str(), F_OK) != 0) {
    if (!MakeDir(image_folder_path_)) {
      return false;
    }
  } else if (access(image_folder_path_.c_str(), F_OK | W_OK) != 0) {
    return false;
  }
  return true;
}

bool NBSfM::CheckFeature() {
  bool is_can_read = access(feature_folder_path_.c_str(), F_OK | R_OK) == 0;
  bool is_can_write = access(feature_folder_path_.c_str(), F_OK | W_OK) == 0;

  if (is_can_read) {
    // Try to read feature files
    num_features_ = 0;
    feature_paths_.clear();

    StringVec str_vec;
    ReadDirectory(feature_folder_path_, str_vec);
    sort(str_vec.begin(), str_vec.end());

    int feature_count = 0;
    for (unsigned int i = 0; i < str_vec.size(); i++) {
      if (EndsWith(str_vec[i], ".csv")) {
        feature_paths_.push_back(feature_folder_path_ + "/" + str_vec[i]);
        feature_count++;
      }
    }

    if (feature_count != num_images_) {
      if (is_can_write) {
        // Redo feature detection
        num_features_ = 0;
        feature_paths_.clear();
        redo_feature_detection_ = true;
      } else {
        cerr << "Need to detect features but cannot write features to feature_folder : " << feature_folder_path_ << endl;
        return false;
      }
    }
  } else {
    redo_feature_detection_ = true;
  }
  return true;
}

void NBSfM::Help(int argc, char *argv[]) {
  cout << "Usage : " << argv[0] << " [Parameters]" << endl;
  cout << "Parameters : (a duplicate parameter overrides previous one)" << endl;
  cout << "  Input" << endl;
  cout << "    [--workspace_path workspace_path]" << endl;
  cout << "    [--image_paths image1_path image2_path [image3_path ...]]" << endl;
  cout << "    [--image_folder image_folder]" << endl;
  cout << "    [--video video_path image_folder_path [max_num_frames]]" << endl;
  cout << "    [--feature_folder feature_folder]" << endl;
  cout << endl;
  cout << "  Recalculate each step. The following steps will be calculated also." << endl;
  cout << "    [--redo_feature_detection]" << endl;
}

inline bool NBSfM::EndsWith(std::string const & value, std::string const & ending) {
  // https://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void NBSfM::ReadDirectory(const std::string& name, StringVec& v) {
  DIR* dirp = opendir(name.c_str());
  struct dirent * dp;
  while ((dp = readdir(dirp)) != NULL) {
    v.push_back(dp->d_name);
  }
  closedir(dirp);
}

bool NBSfM::MakeDir(string path) {
  string mkdir_cmd = "mkdir -p " + path;
  const int dir_err = system(mkdir_cmd.c_str());
  if (-1 == dir_err) {
    cerr << "Error creating directory!n";
    return false;
  }
  return true;
}

vector< vector < string > > NBSfM::ReadCSV(string csv_path) {
  vector< vector < string > > data_list;
  string line = "";

  ifstream file(csv_path);
  while (getline(file, line)) {
    vector< string > vec;
    istringstream ss(line);
    while (ss) {
      string elem;
      if (!getline(ss, elem, ','))
        break;
      try {
        vec.push_back(elem);
      }
      catch (const std::invalid_argument e) {
        e.what();
      }
    }
    data_list.push_back(vec);
  }
  file.close();
  return data_list;
}

bool NBSfM::CSVStr2Mat(vector< vector < string > > data, Mat& mat) {
  // Check size
  unsigned int rows = data.size();
  if (rows <= 0)
    return false;
  unsigned int cols = data[0].size();
  for (unsigned int idx = 1; idx < rows; idx++) {
    if (cols != data[idx].size()) {
      return false;
    }
  }

  // Store data
  mat = Mat::zeros(rows, cols, CV_64F);
  for (unsigned int r = 0; r < rows; r++) {
    for (unsigned int c = 0; c < cols; c++) {
      mat.at<double>(r, c) = stof(data[r][c]);
    }
  }
  return true;
}

bool NBSfM::ExportVideoFrames() {
  cout << endl << endl << "Export frames from a video.." << endl;

  // Clear image paths
  num_images_ = 0;
  image_paths_.clear();

  // Get video
  VideoCapture cap(video_path_.c_str());
  if(!cap.isOpened()) {
    cout << "  Cannot read video." << endl;
    return false;
  }
  int num_frames = cap.get(CV_CAP_PROP_FRAME_COUNT);
  num_frames = min(num_frames, max_num_frames_);
  for (int i = 0; i < num_frames; i++) {
    cout << "  Exporting image " << (i + 1) << "/" << num_frames << endl;

    // Get a new frame from camera
    Mat frame;
    cap >> frame;

    // Write frames
    std::ostringstream ss;
    ss << image_folder_path_ << "/" << std::setw(4) << std::setfill('0') << i << ".png";
    string frame_path(ss.str());
    imwrite(frame_path, frame);
    image_paths_.push_back(ss.str());
    num_images_++;
  }
  return true;
}

bool NBSfM::LoadImages() {
  cout << endl << endl << "Load images.." << endl;
  int img_idx = 0;
  for (auto image_path : image_paths_) {
    cv::Mat img = cv::imread(image_path, CV_LOAD_IMAGE_COLOR);
    if (!img.data) {
      throw runtime_error("Could not open image.");
    }
    if (img_idx == 0) {
      image_width_ = img.cols;
      image_height_ = img.rows;
    } else {
      if (image_width_ != img.cols || image_height_ != img.rows) {
        throw runtime_error("An image doesn't have the same size.");
      }
    }
    images_.push_back(img);
    cout << "  " << img_idx + 1 << " / " << num_images_ << endl;
    img_idx++;
  }
  return true;
}

bool NBSfM::WriteReferenceImage() {
  cout << endl << endl << "Write reference frame.." << endl;
  imwrite(workspace_path_ + "/ReferenceImage.png", images_.at(0));
  return true;
}

bool NBSfM::LoadFeatures() {
  cout << endl << endl << "Load features.." << endl;
  num_features_ = 0;

  int feature_count = 0;
  for (auto feature_path : feature_paths_) {
    vector< vector < string > > data_i = ReadCSV(feature_path);
    Mat features_i;
    CSVStr2Mat(data_i, features_i);

    if (feature_count == 0) {
      num_features_ = features_i.cols;
      features_ = Mat::zeros(2 * num_images_, num_features_, CV_64F);
    }
    features_i.rowRange(0, 2).copyTo(features_.rowRange(feature_count * 2, feature_count * 2 + 2));
    feature_count++;
  }

  if (feature_count != num_images_) {
    return false;
  }
  num_features_ = feature_count;
  return true;
}

bool NBSfM::FeatureDetection() {
  cout << endl << endl << "Feature detection.." << endl;
  Mat image_ref = images_.at(0);
  Mat gray_ref;
  cvtColor(image_ref, gray_ref, CV_RGB2GRAY);

  vector< Point2f > feature_ref;
  goodFeaturesToTrack(gray_ref, feature_ref, 20000, 1e-10, 5, noArray(), 10);
  num_features_ = feature_ref.size();

  features_ = Mat::zeros(2, num_features_, CV_64F);
  for (int i = 0; i < num_features_; i++) {
    features_.at< double >(0, i) = feature_ref[i].x;
    features_.at< double >(1, i) = feature_ref[i].y;
  }
  return true;
}

bool NBSfM::WriteFeatures() {
  cout << endl << endl << "Write feature.." << endl;

  if (access(feature_folder_path_.c_str(), F_OK | W_OK) != 0) {
    // Make directory
    MakeDir(feature_folder_path_);
  } else {
    // Delete old features
    string cmd = "rm " + feature_folder_path_ + "/*.csv";
    system(cmd.c_str());
  }

  // Write csv
  ofstream file(feature_folder_path_ + "/reference.csv");
  file << features_.at<double>(0, 0);
  for (int i = 1; i < num_features_; i++) {
    file << "," << features_.at<double>(0, i);
  }
  file << endl;
  file << features_.at<double>(1, 0);
  for (int i = 1; i < num_features_; i++) {
    file << "," << features_.at<double>(1, i);
  }
  file.close();
  return true;
}

bool NBSfM::WriteFeatureImage() {
  Mat img;
  images_.at(0).copyTo(img);

  for (int i = 0; i < num_features_; i++) {
    circle(img,
           Point(features_.at< double >(0, i), features_.at< double >(1, i)),
           1, Scalar( 0, 0, 255 ), -1);
  }
  imwrite(workspace_path_ + "/FeatureImage.png", img);
  return true;
}

int main(int argc, char * argv[]) {
  NBSfM(argc, argv);
  return 0;
}
