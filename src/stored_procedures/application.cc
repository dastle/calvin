/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// Annoying implementation file due to our Singleton design pattern...

#include "stored_procedures/application.h"
#include "stored_procedures/tpcc/tpcc.h"

namespace calvin {

Application* Application::application_instance_ = NULL;
Application* Application::GetApplicationInstance() {
  // TPCC is the default application
  if (application_instance_ == NULL)
    TPCC::InitializeApplication();

  return application_instance_;
}

Application::Application() {
  application_instance_ = this;
}

}  // namespace calvin
