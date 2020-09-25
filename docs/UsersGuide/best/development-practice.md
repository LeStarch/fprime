# Recommended F′ Development Process

The purpose of this guide is to layout the standard F´ development process. This is the process used by most developers
who use F´ and, as such, many of the F´ tools are written to support the stages of the process. This guild will walk
through each step in this process.

The process:
- [High Level Design](#high-level-design)
- [Setup Deployment](#setup-deployment)
- [Develop Ports and Components](#develop-ports-and-components)
    - [Requirements and Design](#design-and-requirements)
    - [Implement](#implement)
    - [Unit Test](#unit-test)
- [Assemble Topology](#assemble-topology)
- [Integration Testing](#integration-testing)

## High Level Design

The first step of the development process is to establish the high-level design for the project. This involves
specifying system-level requirements and a block diagram that represents the key system functionality. Once complete the
project should break this functionality into discreet units of functionality that represent the system. In addition, the
interface between these units should be defined. The units of functionality are Components and the interfaces are
further broken down into discrete call or actions through that interface. These are F´ ports. The full design of the
system of components and ports is the Topology. See: [Ports Components and Topologies](../user/port-comp-top.md)

Next, the project should review the components provided by the F´ framework to see what functionality can be inherited
for free. This usually consists of the basic command and data handling components, the Os layer, drivers, and other Svc
components. Where possible, these components should be used as-is to support a project to minimize extra work, but these
may be cloned-and-owned if they fall short of project requirements.

The project now has a list of what components they must provide, and what they will inherit.

## Setup Deployment

The next step for most projects is to prepare for development. This means getting enough of a deployment setup such that
the developers can be assigned components and ports to implement, and test within a working deployment.

Typically at this stage the Ref application can be cloned.  Specifically, the `CMakeLists.txt` and the `settings.ini`
files need to be created to provide a buildable system to developers.

## Develop Ports and Components

Next, each developer is typically assigned a set of Components to develop. This development starts with the Ports that
are defined to be used by the Component, and then the Component itself.  This development is described in the following
sections.

### Design and Requirements

Each Port and Component should be designed and modeled. This is done in the (Ai.xml or fpp file) provided for the
component. These designs are provided to the autocoder when complete and should meet the specified requirements for the
component.  Often, more specific requirements are broken-out during this stage and the design is made to match them.

The port or component should then be tied into the build system and built using `fprime-util`.  The raw design should
compile directly. However, the user may need to add templates for each user-implemented file.

### Implement

Next, the developer typically runs `fprime-util impl` to produce `-template` files of the hand-coded .cpp and .hpp files.
These can be used as a basis for implementation with all the stubs in place for the developer to implement the design.
Developers then fill in these files and stubs with an implementation that supports the functionality of the design.

The component can then be built as development proceeds to look for errors.  Ports are normally entirely autogenerated
and thus are not implemented

### Unit Test

Along with implementation, unit tests can be templated and implemented to test against the requirements of the component.
These should be developed an run often to ensure the implemented component works.

## Assemble Topology

As components are completed, it is helpful to add them to the topology. As more components are completed, the topology
is slowly built up over time. This can enable integration tests early on in the project. The full deployment should be
built at this stage to ensure that there are no errors.

## Integration Testing

As the topology comes together, it is helpful to write system level tests for subsystems of the overall deployment. This
makes sure that as a system, top-level requirements are met. These tsts are typically written as scripts triggering
functionality through the F´ GDS.

## Conclusion

The user should now be aware of how to follow a standard process for developing F´ applications. At the end of each
stage, it is helpful to review the work completed.