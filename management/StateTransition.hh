#ifndef STATETRANSITION_HH
#define STATETRANSITION_HH

#include "pds/utility/State.hh"
#include "pds/utility/TransitionId.hh"

namespace Pds {
  namespace StateTransition {
    State::Id state(TransitionId::Value id);
    TransitionId::Value transition(State::Id current, State::Id target);
  };
}

#endif
