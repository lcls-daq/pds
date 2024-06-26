
#include "Fsm.hh"
#include "Action.hh"
#include "Response.hh"

#include <stdlib.h>

using namespace Pds;

const char* Fsm::_stateName()
{ 
  static const char* _names[] = {
    "Idle","Mapped","Configured","Begun","Disabled","Enabled"
  };
  return (_state < NumberOf ? _names[_state] : "-Invalid-");
};

Fsm::Fsm() : 
  _state(Idle), 
  _defaultAction(new Action), 
  _defaultResponse(new Response) 
{
  unsigned i;
  for (i=0;i<TransitionId::NumberOf;i++) {
    _action  [i] = _defaultAction;
  }
  for (i=0;i<OccurrenceId::NumberOf;i++) {
    _response[i] = _defaultResponse;
  }
  _response[OccurrenceId::EvrCommand] = new Pass;
}

Fsm::~Fsm() {
  delete _defaultAction;
  delete _defaultResponse;
}

Fsm::State Fsm::_reqState(TransitionId::Value id) {
  switch (id) {
  case TransitionId::Reset: {
    return Idle;
    break;
  }
  case TransitionId::Map: {
    return Mapped;
    break;
  }
  case TransitionId::Configure: {
    return Configured;
    break;
  }
  case TransitionId::BeginRun: {
    return Begun;
    break;
  }
  case TransitionId::BeginCalibCycle: {
    return Disabled;
    break;
  }
  case TransitionId::Enable: {
    return Enabled;
    break;
  }
  case TransitionId::Unmap: {
    return Idle;
    break;
  }
  case TransitionId::Unconfigure: {
    return Mapped;
    break;
  }
  case TransitionId::EndRun: {
    return Configured;
    break;
  }
  case TransitionId::EndCalibCycle: {
    return Begun;
    break;
  }
  case TransitionId::Disable: {
    return Disabled;
    break;
  }
  case TransitionId::L1Accept: {
    return Enabled;
    break;
  }
  default: {
    printf("Request for illegal transition %s\n",TransitionId::name(id));
    return _state;
    break;
  }
  }
}

unsigned Fsm::_allowed(State reqState, TransitionId::Value id) {
  if (id==TransitionId::L1Accept) {
    return _state==Enabled;
  } else if (id==TransitionId::Reset) {
    return true;
  }
  else {
    return (abs(static_cast<int>(reqState-_state))==1);
  }
}

InDatagram* Fsm::events(InDatagram* in) {
  TransitionId::Value id = in->datagram().seq.service();
  State reqState = _reqState(id);
  if (_allowed(reqState,id)) {
    in = _action[id]->fire(in);
    // need to check for failure of the action before updating _state
    _state = reqState;
  } else {
    // assert invalid transition damage here
    printf("Fsm: Invalid event %s while in state %s\n",TransitionId::name(id),
           _stateName());
  }
  return in;
}

Occurrence* Fsm::occurrences(Occurrence* occ) {
  return _response[occ->id()]->fire(occ);
}

Transition* Fsm::transitions(Transition* tr) {
  TransitionId::Value id = tr->id();
  State reqState = _reqState(id);
  if (_allowed(reqState,id)) {
    tr = _action[id]->fire(tr);
    if (id==TransitionId::Unmap)  // deallocate must transition here
      _state = reqState;
  } else {
    // assert invalid transition damage here
    printf("Fsm: Invalid transition %s while in state %s\n",
           TransitionId::name(id),_stateName());
  }
  return tr;
}

Action* Fsm::callback(TransitionId::Value id, Action* action) {
  Action* oldAction = _action[id];
  _action[id]=action;
  return oldAction;
}

Response* Fsm::callback(OccurrenceId::Value id, Response* response) {
  Response* oldResponse = _response[id];
  _response[id]=response;
  return oldResponse;
}
