# Changes to Gazebo on this branch
- Fixes the transport library error, see 
  https://bitbucket.org/osrf/gazebo/issues/821/apparent-transport-race-condition-on
  https://bitbucket.org/osrf/gazebo/issues/1720/race-condition-in-transport-library
- Fixes the empty message error, see:
  https://bitbucket.org/osrf/gazebo/issues/1722/empty-message-types-caused-by-entity-fini
- Implements a workaround for model delete race condition, see:
  https://bitbucket.org/osrf/gazebo/issues/1739/race-condition-when-deleting-a-model
- Adds a fix for the contact sensor race condition:
  https://bitbucket.org/osrf/gazebo/issues/1740/race-condition-when-deleting-contactsensor
- Changing `Base::parent` to a weak pointer:
  https://bitbucket.org/osrf/gazebo/issues/1142/potential-for-smart-pointer-reference
- Introducing `ODEModel` in order to track a model's collision space in ODE, currently it is not
  removed adequately:
  https://bitbucket.org/osrf/gazebo/issues/1780/ode-collision-space-not-deleted-when-model
- Changing the parent / child joint lists in `Link` to use weak pointers to prevent circular
  references (no issue yet).
- Changing `Joint::model` to use a weak pointer to prevent circular references.
- Changing `JointControllerPrivate::model` to use a weak pointer to prevent circular references.
