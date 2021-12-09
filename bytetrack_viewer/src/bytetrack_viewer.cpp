#include "bytetrack_viewer/bytetrack_viewer.hpp"

namespace bytetrack_viewer{
    cv::Scalar getColor(int id){
        int idx = id + 3;
        return cv::Scalar(37 * idx % 255, 17 * idx % 255, 29 * idx % 255);
    }
    void drawObject(cv::Mat frame, bboxes_ex_msgs::msg::BoundingBox bbox){
        // draw bbox
        auto color = getColor(bbox.id);
        cv::rectangle(frame, 
                    cv::Rect(bbox.ymin, bbox.xmin, 
                            bbox.ymax - bbox.ymin, bbox.xmax - bbox.xmin),
                    color, 2);

        // draw ID
        float brightness = color[2] * 0.3 + color[1] * 0.59 + color[0] * 0.11;
        cv::Scalar txt_color;
        if (brightness > 127){
            txt_color = cv::Scalar(0, 0, 0);
        }else{
            txt_color = cv::Scalar(255, 255, 255);
        }

        std::string txt = cv::format("ID:%d", bbox.id);
        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(txt, cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &baseLine);
        cv::rectangle(frame, 
                      cv::Rect(cv::Point(bbox.ymin, bbox.xmin - label_size.height), 
                               cv::Size(label_size.width, label_size.height + baseLine)),
                      color, -1);
        cv::putText(frame, txt,
                    cv::Point(bbox.ymin, bbox.xmin), 
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, txt_color, 1, cv::LINE_AA);
    }

    ByteTrackViewer::ByteTrackViewer(const std::string &node_name, const rclcpp::NodeOptions& options)
    : rclcpp::Node("bytetrack_viewer", node_name, options)
    {
    }
    ByteTrackViewer::ByteTrackViewer(const rclcpp::NodeOptions& options)
    : ByteTrackViewer("", options)
    {
        this->initializeParameter_();

        if (this->use_exact_sync_) {
            this->exact_sync_ = std::make_shared<ExactSynchronizer>(
                ExactSyncPolicy(this->queue_size_),
                sub_image_,
                sub_bboxes_);
            this->exact_sync_->registerCallback(
                std::bind(
                    &ByteTrackViewer::imageCallback,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2));
        } else {
            this->sync_ = std::make_shared<Synchronizer>(
                SyncPolicy(this->queue_size_), sub_image_, sub_bboxes_);
            this->sync_->registerCallback(
                std::bind(
                    &ByteTrackViewer::imageCallback,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2));
        }

        connectCallback();
        std::lock_guard<std::mutex> lock(this->connect_mutex_);
        cv::namedWindow("ByteTrackViewer", cv::WINDOW_AUTOSIZE);

    }
    void ByteTrackViewer::initializeParameter_()
    {
        this->queue_size_ = this->declare_parameter<int>("queue_size", 5);
        this->use_exact_sync_ = this->declare_parameter<bool>("exact_sync", false);
        this->sub_image_topic_name_ = this->declare_parameter<std::string>("sub_image_topic_name", "/image_raw");
        this->sub_bboxes_topic_name_ = this->declare_parameter<std::string>("sub_bboxes_topic_name", "/bytetrack/bounding_boxes");
    }
    void ByteTrackViewer::connectCallback()
    {
        std::lock_guard<std::mutex> lock(this->connect_mutex_);
        if (0) {
            this->sub_image_.unsubscribe();
            this->sub_bboxes_.unsubscribe();
        } else if (!this->sub_image_.getSubscriber()) {
            image_transport::TransportHints hints(this, "raw");
            this->sub_image_.subscribe(this, this->sub_image_topic_name_, hints.getTransport());
            this->sub_bboxes_.subscribe(this, this->sub_bboxes_topic_name_);
        }
    }
    void ByteTrackViewer::imageCallback(
        const sensor_msgs::msg::Image::ConstSharedPtr & image_msg,
        const bboxes_ex_msgs::msg::BoundingBoxes::ConstSharedPtr & trackers_msg)
    {
        auto img = cv_bridge::toCvCopy(image_msg, "bgr8");
        cv::Mat frame = img->image;

        auto bboxes = trackers_msg->bounding_boxes;
        for(auto bbox: bboxes){
            drawObject(frame, bbox);
        }
        cv::imshow("ByteTrackViewer", frame);
        auto key = cv::waitKey(1);
        if(key == 27){
            rclcpp::shutdown();
        }
    }
}

#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(bytetrack_viewer::ByteTrackViewer)