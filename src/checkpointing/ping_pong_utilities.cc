/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Since Ping-Pong is so incredibly fc*kin complicated, we include a set of
/// utilities as a separate file, mostly because they are not interesting.

#include "checkpointing/ping_pong.h"

namespace calvin {

Value* PingPong::RetrieveValue(const Key& key, ValueLocation location,
                               int key_override) {
  int int_key
    = (key_override >= 0 ?
       key_override : Application::GetApplicationInstance()->KeyToInt(key));

  switch (level_) {
    case NONE:
      switch (location) {
        case APP_STATE:
          return application_state_[int_key];
        case ODD:
          return odd_[int_key].value;
        case EVEN:
          return even_[int_key].value;
      }
      break;

    case INTERLEAVED:
      switch (location) {
        case APP_STATE:
          return inter_db_[int_key].app_value;
        case ODD:
          return inter_db_[int_key].odd_value;
        case EVEN:
          return inter_db_[int_key].even_value;
      }
      break;

    case HASH_MAP:
      switch (location) {
        case APP_STATE:
          return application_hash_map_.Get(key);
          break;
        case ODD:
          if (!odd_hash_map_.Contains(key))
            return NULL;
          return odd_hash_map_.Get(key)->value;
          break;
        case EVEN:
          if (!even_hash_map_.Contains(key))
            return NULL;
          return even_hash_map_.Get(key)->value;
          break;
      }
      break;

    case INTERLEAVED_HASH_MAP:
      if (!interleaved_hash_map_.Contains(key))
        return NULL;
      switch (location) {
        case APP_STATE:
          return interleaved_hash_map_.Get(key)->app_value;
          break;
        case ODD:
          return interleaved_hash_map_.Get(key)->odd_value;
          break;
        case EVEN:
          return interleaved_hash_map_.Get(key)->even_value;
          break;
      }
      break;
  }

  return NULL;
}

bool PingPong::Dirty(const Key& key, ValueLocation location, int key_override) {
  int int_key
    = (key_override >= 0 ?
       key_override : Application::GetApplicationInstance()->KeyToInt(key));

  switch (level_) {
    case NONE:
      switch (location) {
        case ODD:
          return odd_[int_key].dirty;
        case EVEN:
          return even_[int_key].dirty;
        default:
          return false;
      }
      break;

    case INTERLEAVED:
      switch (location) {
        case ODD:
          return inter_db_[int_key].odd_dirty;
        case EVEN:
          return inter_db_[int_key].even_dirty;
        default:
          return false;
      }
      break;

    case HASH_MAP:
      switch (location) {
        case ODD:
          if (!odd_hash_map_.Contains(key))
            return false;
          return odd_hash_map_.Get(key)->dirty;
          break;
        case EVEN:
          if (!even_hash_map_.Contains(key))
            return false;
          return even_hash_map_.Get(key)->dirty;
          break;
        default:
          break;
      }
      break;

    case INTERLEAVED_HASH_MAP:
      if (!interleaved_hash_map_.Contains(key))
        return NULL;
      switch (location) {
        case ODD:
          return interleaved_hash_map_.Get(key)->odd_dirty;
          break;
        case EVEN:
          return interleaved_hash_map_.Get(key)->even_dirty;
          break;
        default:
          break;
      }
      break;
  }

  return false;
}

void PingPong::SetValue(const Key& key, Value* value, ValueLocation location,
                        int key_override) {
  int int_key
    = (key_override >= 0 ?
       key_override : Application::GetApplicationInstance()->KeyToInt(key));

  switch (level_) {
    case NONE:
      switch (location) {
        case APP_STATE:
          if (!application_state_[int_key])
            total_inserts_++;
          application_state_[int_key] = value;
          break;
        case ODD:
          odd_[int_key].value = value;
          break;
        case EVEN:
          even_[int_key].value = value;
          break;
      }
      break;

    case INTERLEAVED:
      switch (location) {
        case APP_STATE:
          if (!inter_db_[int_key].app_value)
            total_inserts_++;
          inter_db_[int_key].app_value = value;
          break;
        case ODD:
          inter_db_[int_key].odd_value = value;
          break;
        case EVEN:
          inter_db_[int_key].even_value = value;
          break;
      }
      break;

    case HASH_MAP:
      switch (location) {
        case APP_STATE:
          if (!application_hash_map_.Contains(key))
            application_hash_map_.Insert(key, value);
          else
            application_hash_map_.Set(key, value);
          break;
        case ODD:
          if (!odd_hash_map_.Contains(key))
            odd_hash_map_.Insert(key, new OddEvenRow());
          odd_hash_map_.Get(key)->value = value;
          break;
        case EVEN:
          if (!even_hash_map_.Contains(key))
            even_hash_map_.Insert(key, new OddEvenRow());
          even_hash_map_.Get(key)->value = value;
          break;
      }
      break;

    case INTERLEAVED_HASH_MAP:
      if (!interleaved_hash_map_.Contains(key))
        interleaved_hash_map_.Insert(key, new IPPRow());
      switch (location) {
        case APP_STATE:
          interleaved_hash_map_.Get(key)->app_value = value;
          break;
        case ODD:
          interleaved_hash_map_.Get(key)->odd_value = value;
          break;
        case EVEN:
          interleaved_hash_map_.Get(key)->even_value = value;
          break;
      }
      break;
  }
}

void PingPong::SetDirtyBit(const Key& key, ValueLocation location, bool value,
                           int key_override) {
  int int_key
    = (key_override >= 0 ?
       key_override : Application::GetApplicationInstance()->KeyToInt(key));

  switch (level_) {
    case NONE:
      switch (location) {
        case ODD:
          odd_[int_key].dirty = value;
          break;
        case EVEN:
          even_[int_key].dirty = value;
          break;
        default:
          break;
      }
      break;

    case INTERLEAVED:
      switch (location) {
        case ODD:
          inter_db_[int_key].odd_dirty = value;
          break;
        case EVEN:
          inter_db_[int_key].even_dirty = value;
          break;
        default:
          break;
      }
      break;

    case HASH_MAP:
      switch (location) {
        case ODD:
          if (!odd_hash_map_.Contains(key))
            odd_hash_map_.Insert(key, new OddEvenRow());
          odd_hash_map_.Get(key)->dirty = value;
          break;
        case EVEN:
          if (!even_hash_map_.Contains(key))
            even_hash_map_.Insert(key, new OddEvenRow());
          even_hash_map_.Get(key)->dirty = value;
          break;
        default:
          break;
      }
      break;

    case INTERLEAVED_HASH_MAP:
      if (!interleaved_hash_map_.Contains(key))
        interleaved_hash_map_.Insert(key, new IPPRow());
      switch (location) {
        case ODD:
          interleaved_hash_map_.Get(key)->odd_dirty = value;
          break;
        case EVEN:
          interleaved_hash_map_.Get(key)->even_dirty = value;
          break;
        default:
          break;
      }
      break;
  }
}

}  // namespace calvin
