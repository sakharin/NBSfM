#include "NBSfM.hpp"

NBSfM::NBSfM(int argc, char *argv[]) :
    workspace_path_("."),
    image_folder_path_("./images"),
    video_path_(""),
    max_num_frames_(30),

    is_use_images_(false),
    is_use_video_(false),

    feature_folder_path_("./features"),
    matched_feature_folder_path_("./matched_features"),

    redo_feature_detection_(false),
    redo_feature_matching_(false),

    num_features_(0),
    num_matched_features_(0) {
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
  } else {
    if (!LoadFeatures()) {
      cerr << "Cannot load features." << endl;
      exit(-1);
    }
  }

  if (redo_feature_matching_) {
    FeatureMatching();
    WriteFeatures();
    WriteMatchedFeatures();
  } else {
    if (!LoadMatchedFeatures()) {
      cerr << "Cannot load matched features." << endl;
      exit(-1);
    }
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
      image_names_.clear();

      index += 1;
      while (index < argc && string(argv[index]).compare(0, 2, "--") != 0) {
        if (!IsFileReadable(argv[index])) {
          cerr << "Cannot read an image : " << argv[index] << "." << endl;
          return false;
        }
        image_paths_.push_back(argv[index]);
        if (!CheckImage(image_paths_.back())) {
          cerr << "Cannot read an image : " << argv[index] << "." << endl;
          return false;
        }
        image_names_.push_back(GetNameFromPath(image_paths_.back()));
        num_images_++;
        index += 1;
      }
      is_use_video_ = false;
      is_use_images_ = true;
    } else if (index + 1 < argc && strcmp(argv[index], "--image_folder") == 0) {
      image_folder_path_.assign(argv[index + 1]);
      if (!IsFileReadable(image_folder_path_)) {
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
      double max_num_frames = strtod(argv[index + 2], NULL);
      if (max_num_frames == 0) {
        // It could be path or number
        string image_folder_path;
        image_folder_path.assign(argv[index + 2]);
        if (!IsFolderExist(image_folder_path) || !IsFileWritable(image_folder_path)) {
          cerr << "Parameter cannot be neither image_folder_path nor max_num_frame : " << argv[index + 2] << "." << endl;
          return false;
        } else {
          image_folder_path_.assign(argv[index + 2]);
          max_num_frames = strtod(argv[index + 3], NULL);
        }
      } else {
        max_num_frames_ = max_num_frames;
        image_folder_path_.assign(argv[index + 3]);
        if (!IsFolderExist(image_folder_path_) || !IsFileWritable(image_folder_path_)) {
          cerr << "Cannot write on image_folder_path : " << image_folder_path_ << "." << endl;
          return false;
        }
      }
      is_use_images_ = false;
      is_use_video_ = true;
      index += 4;
    } else if (index + 2 < argc && strcmp(argv[index], "--video") == 0 &&
               string(argv[index + 1]).compare(0, 2, "--") != 0 &&
               string(argv[index + 2]).compare(0, 2, "--") != 0) {
      video_path_.assign(argv[index + 1]);
      double max_num_frames = strtod(argv[index + 2], NULL);
      if (max_num_frames == 0) {
        // It could be path or number
        string image_folder_path;
        image_folder_path.assign(argv[index + 2]);
        if (!IsFolderExist(image_folder_path) || !IsFileWritable(image_folder_path)) {
          cerr << "Parameter cannot be neither image_folder_path nor max_num_frame : " << argv[index + 2] << "." << endl;
          return false;
        } else {
          image_folder_path_.assign(argv[index + 2]);
        }
      } else {
        if (max_num_frames < 2) {
          cerr << "max_num_frames must not be less than 2." << endl;
          return false;
        } else {
          max_num_frames_ = max_num_frames;
        }
      }
      is_use_images_ = false;
      is_use_video_ = true;
      index += 3;
    } else if (index + 1 < argc && strcmp(argv[index], "--video") == 0 &&
               string(argv[index + 1]).compare(0, 2, "--") != 0) {
      video_path_.assign(argv[index + 1]);
      is_use_images_ = false;
      is_use_video_ = true;
      index += 3;
    } else if (index + 1 < argc && strcmp(argv[index], "--feature_folder") == 0) {
      feature_folder_path_.assign(argv[index + 1]);
      if (!CheckFeatures()) {
        return false;
      }
      index += 2;
    } else if (index + 1 < argc && strcmp(argv[index], "--matched_feature_folder") == 0) {
      matched_feature_folder_path_.assign(argv[index + 1]);
      if (!CheckMatchedFeatures()) {
        return false;
      }
      index += 2;
    } else if (index < argc && strcmp(argv[index], "--redo_feature_detection") == 0) {
      redo_feature_detection_ = true;
      index += 1;
    } else if (index < argc && strcmp(argv[index], "--redo_feature_matching") == 0) {
      redo_feature_matching_ = true;
      index += 1;
    } else {
      Help(argc, argv);
      return false;
    }
  }

  if (!IsFolderExist(workspace_path_) || !IsFileWritable(workspace_path_)) {
    cerr << "Cannot write on workspace_path : " << workspace_path_ << "." << endl;
    return false;
  }
  if (is_use_images_) {
    if (image_paths_.size() < 2) {
        cerr << "At least 2 images are required." << endl;
        return false;
    }
  } else if (is_use_video_) {
    if (!IsFileReadable(video_path_)) {
      cerr << "Cannot read video_path : " << video_path_ << "." << endl;
      return false;
    }
    if (!IsFolderExist(image_folder_path_)) {
      MakeDir(image_folder_path_);
    } else if (!IsFileWritable(image_folder_path_)) {
      cerr << "Cannot write on image_folder_path : " << image_folder_path_ << "." << endl;
      return false;
    }
    if (max_num_frames_ < 2) {
      cerr << "max_num_frames_ must not be less than 2." << endl;
      return false;
    }
  } else {
    cerr << "A video or 2 images are required." << endl;
    return false;
  }

  if (redo_feature_detection_) {
    redo_feature_matching_ = true;
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
  cout << "    matched_feature_folder : " << matched_feature_folder_path_ << endl;
  cout << endl;
  cout << "                 Recalculate each step" << endl;
  cout << "    redo_feature_detection : " << ((redo_feature_detection_) ? "true" : "false") << endl;
  cout << "     redo_feature_matching : " << ((redo_feature_matching_) ? "true" : "false") << endl;
}

bool NBSfM::CheckWorkspace() {
  // Check images
  image_folder_path_.assign(workspace_path_ + "/images");
  if (IsFolderExist(image_folder_path_) && IsFileReadable(image_folder_path_)) {
    CheckImagesInFolder();
    is_use_video_ = false;
    is_use_images_ = true;
  } else {
    num_images_ = 0;
    image_paths_.clear();
  }

  // Check features
  feature_folder_path_.assign(workspace_path_ + "/features");
  CheckFeatures();

  // Check matched_features
  matched_feature_folder_path_.assign(workspace_path_ + "/matched_features");
  CheckMatchedFeatures();
  return true;
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
  image_names_.clear();

  StringVec str_vec;
  ReadDirectory(image_folder_path_, str_vec);
  sort(str_vec.begin(), str_vec.end());
  vector< string > tmp_image_paths;
  vector< string > tmp_image_names;
  for (unsigned int i = 0; i < str_vec.size(); i++) {
    string path = image_folder_path_ + "/" + str_vec[i];
    if (IsFileReadable(path) && CheckImage(path)) {
      tmp_image_paths.push_back(path);
      tmp_image_names.push_back(GetNameFromPath(path));
    }
  }
  if (tmp_image_paths.size() >= 2) {
    image_paths_.clear();
    image_names_.clear();
    for (unsigned int i = 0; i < tmp_image_paths.size(); i++) {
      image_paths_.push_back(tmp_image_paths[i]);
      image_names_.push_back(tmp_image_names[i]);
    }
    num_images_ = image_paths_.size();
  } else {
    return false;
  }
  return true;
}

bool NBSfM::CheckFeatures() {
  bool is_can_read = IsFolderExist(feature_folder_path_) && IsFileReadable(feature_folder_path_);
  bool is_can_write = IsFolderExist(feature_folder_path_) && IsFileWritable(feature_folder_path_);

  if (is_can_read) {
    // Clear data
    num_features_ = 0;
    feature_paths_.clear();

    // Try to read feature files
    int feature_count = 0;
    for (unsigned int i = 0; i < image_names_.size(); i++) {
      string feature_path = feature_folder_path_ + "/" + image_names_[i] + ".csv";
      if (!IsFileReadable(feature_path)) {
        // Cannot read feature file
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
      feature_paths_.push_back(feature_path);
      feature_count++;
    }
  } else {
    redo_feature_detection_ = true;
  }
  return true;
}

bool NBSfM::CheckMatchedFeatures() {
  bool is_can_read = IsFolderExist(matched_feature_folder_path_) && IsFileReadable(matched_feature_folder_path_);
  bool is_can_write = IsFolderExist(matched_feature_folder_path_) && IsFileWritable(matched_feature_folder_path_);

  if (is_can_read) {
    // Clear data
    num_matched_features_ = 0;
    matched_feature_paths_.clear();

    // Try to read feature files
    int matched_feature_count = 0;
    for (unsigned int i = 0; i < image_names_.size(); i++) {
      string feature_path = matched_feature_folder_path_ + "/" + image_names_[i] + ".csv";
      if (!IsFileReadable(feature_path)) {
        // Cannot read feature file
        if (is_can_write) {
          // Redo feature matching
          num_matched_features_ = 0;
          matched_feature_paths_.clear();
          redo_feature_matching_ = true;
        } else {
          cerr << "Need to matched features but cannot write matched features to matched_feature_folder : " << matched_feature_folder_path_ << endl;
          return false;
        }
      }
      matched_feature_paths_.push_back(feature_path);
      matched_feature_count++;
    }
  } else {
    redo_feature_matching_ = true;
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
  cout << "    [--video video_path [image_folder_path] [max_num_frames]]" << endl;
  cout << "    [--feature_folder feature_folder]" << endl;
  cout << "    [--matched_feature_folder matched_feature_folder]" << endl;
  cout << endl;
  cout << "  Recalculate each step. The following steps will be calculated also." << endl;
  cout << "    [--redo_feature_detection]" << endl;
  cout << "    [--redo_feature_matching]" << endl;
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

bool NBSfM::IsFolderExist(string path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    // Path dose not exist
    return false;
  } else if(info.st_mode & S_IFDIR) {
    // Path is a directory
    return true;
  } else {
    return false;
  }
}

bool NBSfM::IsFileReadable(string path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    // Path dose not exist
    return false;
  } else if(info.st_mode & S_IRUSR) {
    // Path is readable
    return true;
  } else {
    return false;
  }
}

bool NBSfM::IsFileWritable(string path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    // Path dose not exist
    return false;
  } else if(info.st_mode & S_IWUSR) {
    // Path is writable
    return true;
  } else {
    return false;
  }
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

string NBSfM::GetNameFromPath(string path) {
  // Remove directory
  size_t last_slash = path.find_last_of("/");
  if (last_slash != string::npos) {
    path = path.substr(last_slash + 1);
  }

  // Remove extension
  size_t last_dot = path.find_last_of(".");
  if (last_dot != string::npos) {
    path = path.substr(0, last_dot);
  }
  return path;
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
    image_names_.push_back(GetNameFromPath(ss.str()));
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
  cout << endl << endl << "Write reference image.." << endl;
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
    if (features_i.rows != 2 || features_i.cols != num_features_) {
      return false;
    }
    features_i.rowRange(0, 2).copyTo(features_.rowRange(feature_count * 2, feature_count * 2 + 2));
    feature_count++;
  }

  if (feature_count != num_images_) {
    return false;
  }
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
  cout << "  Get " << num_features_ << " features." << endl;
  return true;
}

bool NBSfM::WriteFeatures() {
  cout << endl << endl << "Write feature.." << endl;

  if (IsFolderExist(feature_folder_path_)) {
    if (!IsFileWritable(feature_folder_path_)) {
      cout << "Cannot write features." << endl;
      return false;
    }
    // Delete old features
    string cmd = "rm -r " + feature_folder_path_ + "/*.csv";
    system(cmd.c_str());
  } else {
    MakeDir(feature_folder_path_);
  }

  // Write features
  for (int i = 0; i < num_images_; i++) {
    string feature_path = feature_folder_path_ + "/" + image_names_[i] + ".csv";
    int index = i * 2;
    ofstream file(feature_path);
    file << features_.at<double>(index, 0);
    for (int j = 1; j < num_features_; j++) {
      file << "," << features_.at<double>(index, j);
    }
    file << endl;
    index++;
    file << features_.at<double>(index, 0);
    for (int j = 1; j < num_features_; j++) {
      file << "," << features_.at<double>(index, j);
    }
    file.close();
  }
  return true;
}

bool NBSfM::LoadMatchedFeatures() {
  cout << endl << endl << "Load matched_features.." << endl;
  num_matched_features_ = 0;

  int feature_count = 0;
  for (auto feature_path : matched_feature_paths_) {
    vector< vector < string > > data_i = ReadCSV(feature_path);
    Mat features_i;
    CSVStr2Mat(data_i, features_i);

    if (feature_count == 0) {
      num_matched_features_ = features_i.cols;
      matched_features_ = Mat::zeros(2 * num_images_, num_matched_features_, CV_64F);
    }
    if (features_i.rows != 2 || features_i.cols != num_matched_features_) {
      return false;
    }
    features_i.rowRange(0, 2).copyTo(matched_features_.rowRange(feature_count * 2, feature_count * 2 + 2));
    feature_count++;
  }

  if (feature_count != num_images_) {
    return false;
  }
  return true;
}

bool NBSfM::FeatureMatching() {
  cout << endl << endl << "Feature matching.." << endl;

  // Reference image
  Mat img_gray_0;
  cvtColor(images_.at(0), img_gray_0, CV_RGB2GRAY);
  vector< Point2f > features_0;
  for (int i = 0; i < num_features_; i++) {
    features_0.push_back(Point2f(features_.at<double>(0, i),
                                 features_.at<double>(1, i)));
  }

  // Get feature matching
  Mat mask = Mat::ones(num_images_, num_features_, CV_8U);
  vector< vector< Point2f > > features;
  for (int i = 0; i < num_images_; i++) {
    vector< Point2f > tmp;
    features.push_back(tmp);
    Mat img_gray_i;
    cvtColor(images_.at(i), img_gray_i, CV_RGB2GRAY);

    vector< Point2f > features_forward;
    vector< Point2f > features_backward;
    vector< unsigned char > status_forward;
    vector< unsigned char > status_backward;
    vector< float > error_forward;
    vector< float > error_backward;

    calcOpticalFlowPyrLK(img_gray_0, img_gray_i, features_0, features_forward,
                         status_forward, error_forward);
    calcOpticalFlowPyrLK(img_gray_i, img_gray_0, features_forward, features_backward,
                         status_backward, error_backward);

    for (int j = 0; j < num_features_; j++) {
      // Store features
      features[i].push_back(features_forward[j]);

      // Compute mask
      float bidirectional_error = norm(features_0.at(j) - features_backward.at(j));
      if (status_forward[j] == 0 || status_backward[j] == 0 || bidirectional_error > 0.1) {
        mask.at<unsigned char>(i, j) = 0;
      }
    }
  }

  // Filter features which appears on all images
  Mat count_mask = Mat::zeros(1, num_features_, CV_32S);
  num_matched_features_ = 0;
  for (int j = 0; j < num_features_; j++) {
    int count = 1; // All features are on reference image, no need to count them
    for (int i = 1; i < num_images_; i++) {
      if (mask.at<unsigned char>(i, j) == 1) {
        count++;
      }
    }
    count_mask.at<int>(j) = count;
    if (count == num_images_) {
      num_matched_features_++;
    }
  }
  if (num_matched_features_ < 10) {
    return false;
  }

  // Copy features to features_ and matched_features_
  features_ = Mat::zeros(2 * num_images_, num_features_, CV_64F);
  matched_features_ = Mat::zeros(2 * num_images_, num_matched_features_, CV_64F);
  for (int i = 0; i < num_images_; i++) {
    int index = i * 2;
    int idx = 0;
    for (int j = 0; j < num_features_; j++) {
      Point2f p = features[i][j];
      features_.at<double>(index,     j) = p.x;
      features_.at<double>(index + 1, j) = p.y;

      if (count_mask.at<int>(j) == num_images_) {
        matched_features_.at<double>(index,     idx) = p.x;
        matched_features_.at<double>(index + 1, idx) = p.y;
        idx++;
      }
    }
  }
  cout << "  Get " << num_matched_features_ << " tracked features."  << endl;
  return true;
}

bool NBSfM::WriteMatchedFeatures() {
  cout << endl << endl << "Write matched features.." << endl;

  if (IsFolderExist(matched_feature_folder_path_)) {
    if (!IsFileWritable(matched_feature_folder_path_)) {
      cout << "Cannot write matched features." << endl;
      return false;
    }
    // Delete old features
    string cmd = "rm " + matched_feature_folder_path_ + "/*.csv";
    system(cmd.c_str());
  } else {
    MakeDir(matched_feature_folder_path_);
  }

  // Write features
  for (int i = 0; i < num_images_; i++) {
    string feature_path = matched_feature_folder_path_ + "/" + image_names_[i] + ".csv";
    int index = i * 2;
    ofstream file(feature_path);
    file << matched_features_.at<double>(index, 0);
    for (int j = 1; j < num_matched_features_; j++) {
      file << "," << matched_features_.at<double>(index, j);
    }
    file << endl;
    index++;
    file << matched_features_.at<double>(index, 0);
    for (int j = 1; j < num_matched_features_; j++) {
      file << "," << matched_features_.at<double>(index, j);
    }
    file.close();
  }
  return true;
}

bool NBSfM::WriteFeatureImage() {
  cout << endl << endl << "Write feature image.." << endl;
  Mat img;
  images_.at(0).copyTo(img);

  for (int i = 0; i < num_features_; i++) {
    circle(img,
           Point(features_.at< double >(0, i), features_.at< double >(1, i)),
           1, Scalar(0, 0, 255), -1);
  }
  for (int i = 0; i < num_matched_features_; i++) {
    circle(img,
           Point(matched_features_.at< double >(0, i),
                 matched_features_.at< double >(1, i)),
           1, Scalar(0, 255, 0), -1);
  }
  imwrite(workspace_path_ + "/FeatureImage.png", img);
  return true;
}

int main(int argc, char * argv[]) {
  NBSfM(argc, argv);
  return 0;
}
