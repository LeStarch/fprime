



    // ------------------------------------------------------------------------------------------------------
    // Rule:  Create
    //
    // ------------------------------------------------------------------------------------------------------
    struct Create : public STest::Rule<Os::Test::Queue::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            Create();

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Test::Queue::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Test::Queue::Tester& state //!< The test state
            );

    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  Send
    //
    // ------------------------------------------------------------------------------------------------------
    struct Send : public STest::Rule<Os::Test::Queue::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            Send();

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Test::Queue::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Test::Queue::Tester& state //!< The test state
            );

    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  Receive
    //
    // ------------------------------------------------------------------------------------------------------
    struct Receive : public STest::Rule<Os::Test::Queue::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            Receive();

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Test::Queue::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Test::Queue::Tester& state //!< The test state
            );

    };

    





    // ------------------------------------------------------------------------------------------------------
    // Rule:  Overflow
    //
    // ------------------------------------------------------------------------------------------------------
    struct Overflow : public STest::Rule<Os::Test::Queue::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            Overflow();

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Test::Queue::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Test::Queue::Tester& state //!< The test state
            );

    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  Underflow
    //
    // ------------------------------------------------------------------------------------------------------
    struct Underflow : public STest::Rule<Os::Test::Queue::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            Underflow();

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Test::Queue::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Test::Queue::Tester& state //!< The test state
            );

    };

    