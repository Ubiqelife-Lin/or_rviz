#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/map.hpp>
#include "KinBodyMarker.h"

using boost::ref;
using boost::format;
using boost::str;
using boost::algorithm::ends_with;
using boost::adaptors::map_values;
using OpenRAVE::EnvironmentBasePtr;
using OpenRAVE::KinBodyPtr;
using OpenRAVE::KinBodyWeakPtr;
using OpenRAVE::RobotBase;
using OpenRAVE::RobotBaseWeakPtr;
using visualization_msgs::InteractiveMarkerFeedbackConstPtr;
using interactive_markers::InteractiveMarkerServer;
using interactive_markers::MenuHandler;

typedef OpenRAVE::KinBody::LinkPtr LinkPtr;
typedef OpenRAVE::RobotBase::ManipulatorPtr ManipulatorPtr;
typedef boost::shared_ptr<InteractiveMarkerServer> InteractiveMarkerServerPtr;
typedef MenuHandler::EntryHandle EntryHandle;


namespace or_interactivemarker {

// TODO: Move this to a helper header.
static MenuHandler::CheckState BoolToCheckState(bool const &flag)
{
    if (flag) {
        return MenuHandler::CHECKED;
    } else {
        return MenuHandler::UNCHECKED;
    }
}

static bool CheckStateToBool(MenuHandler::CheckState const &state)
{
    return state == MenuHandler::CHECKED;
}


KinBodyMarker::KinBodyMarker(InteractiveMarkerServerPtr server,
                             KinBodyPtr kinbody)
    : server_(server)
    , kinbody_(kinbody)
    , robot_(boost::dynamic_pointer_cast<RobotBase>(kinbody))
{
    BOOST_ASSERT(server);
    BOOST_ASSERT(kinbody);

#if 0
    if (!IsGhost()) {
        CreateGhost();
    }
#endif
}

KinBodyMarker::~KinBodyMarker()
{
    if (ghost_kinbody_) {
        ghost_kinbody_->GetEnv()->Remove(ghost_kinbody_);
        ghost_kinbody_.reset();
        ghost_robot_.reset();
    }
}

bool KinBodyMarker::IsGhost() const
{
    KinBodyPtr kinbody = kinbody_.lock();
    return ends_with(kinbody->GetName(), ".Ghost");
}

void KinBodyMarker::EnvironmentSync()
{
    typedef OpenRAVE::KinBody::LinkPtr LinkPtr;
    typedef OpenRAVE::KinBody::JointPtr JointPtr;

    KinBodyPtr const kinbody = kinbody_.lock();
    bool const is_ghost = IsGhost();

    // Update links. This includes the geometry of the KinBody.
    for (LinkPtr link : kinbody->GetLinks()) {
        LinkMarkerWrapper &wrapper = link_markers_[link.get()];
        LinkMarkerPtr &link_marker = wrapper.link_marker;
        if (!link_marker) {
            link_marker = boost::make_shared<LinkMarker>(server_, link, is_ghost);
            CreateMenu(wrapper);
            UpdateMenu(wrapper);
        }
        link_marker->EnvironmentSync();
    }

#if 0
    // Update joints.
    for (JointPtr joint : kinbody_->GetJoints()) {
        JointMarkerPtr &joint_marker = joint_markers_[joint.get()];
        if (!joint_marker) {
            joint_marker = boost::make_shared<JointMarker>(server_, joint);
        }
        joint_marker->EnvironmentSync();
    }

    // Also update manipulators if we're a robot.
    if (robot_ && !ManipulatorMarker::IsGhost(kinbody_)) {
        for (ManipulatorPtr const manipulator : robot_->GetManipulators()) {
            auto const it = manipulator_markers_.find(manipulator.get());
            BOOST_ASSERT(it != manipulator_markers_.end());
            it->second->EnvironmentSync();
        }
    }
#endif
}

void KinBodyMarker::CreateMenu(LinkMarkerWrapper &link_wrapper)
{
    BOOST_ASSERT(!link_wrapper.has_menu);

    auto const cb = boost::bind(&KinBodyMarker::MenuCallback, this,
                                ref(link_wrapper), _1);

    MenuHandler &menu_handler = link_wrapper.link_marker->menu_handler();
    EntryHandle parent = menu_handler.insert("Body");
    link_wrapper.menu_parent = parent;
    link_wrapper.menu_enabled = menu_handler.insert(parent, "Enabled", cb);
    link_wrapper.menu_visible = menu_handler.insert(parent, "Visible", cb);
    link_wrapper.menu_joints = menu_handler.insert(parent, "Joint Controls", cb);
    link_wrapper.has_menu = true;
}

void KinBodyMarker::UpdateMenu(LinkMarkerWrapper &link_wrapper)
{
    if (!link_wrapper.has_menu) {
        return;
    }

    MenuHandler &menu_handler = link_wrapper.link_marker->menu_handler();
    LinkPtr const link = link_wrapper.link_marker->link();

    menu_handler.setCheckState(link_wrapper.menu_enabled,
        BoolToCheckState(link->IsEnabled()));
    menu_handler.setCheckState(link_wrapper.menu_visible,
        BoolToCheckState(link->IsVisible()));
}

void KinBodyMarker::MenuCallback(LinkMarkerWrapper &link_wrapper,
                                 InteractiveMarkerFeedbackConstPtr const &feedback)
{
    RAVELOG_INFO("Callback for %s\n", link_wrapper.link_marker->id().c_str());

#if 0
    if (menus_visible_.count(feedback->menu_entry_id)) {
    }
#endif

    // TODO: Implement this.
}

void KinBodyMarker::CreateGhost()
{
    KinBodyPtr kinbody = kinbody_.lock();

    EnvironmentBasePtr env = kinbody->GetEnv();
    if (kinbody->IsRobot()) {
        ghost_robot_ = OpenRAVE::RaveCreateRobot(env, "");
        ghost_kinbody_ = ghost_robot_;
    } else {
        ghost_kinbody_ = OpenRAVE::RaveCreateKinBody(env, "");
    }

    ghost_robot_->Clone(kinbody, OpenRAVE::Clone_Bodies);
    ghost_robot_->SetName(kinbody->GetName() + ".Ghost");
    ghost_robot_->Enable(false);
    env->Add(ghost_kinbody_, true);
}

}
