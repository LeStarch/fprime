// ----------------------------------------------------------------------
// Tester.hpp
// ----------------------------------------------------------------------

#ifndef TESTER_HPP
#define TESTER_HPP

#include "../../LinuxTimeImpl.hpp"
#include "TimeGTestBase.hpp"

namespace Svc {

  class Tester :
    public TimeGTestBase
  {

      // ----------------------------------------------------------------------
      // Construction and destruction
      // ----------------------------------------------------------------------

    public:

      Tester(const char *const compName);

      ~Tester();

      // ----------------------------------------------------------------------
      // Tests
      // ----------------------------------------------------------------------

    public:

      void getTime();

      // ----------------------------------------------------------------------
      // The component under test
      // ----------------------------------------------------------------------

    private:

      LinuxTimeImpl linuxTime;

  };

};

#endif
