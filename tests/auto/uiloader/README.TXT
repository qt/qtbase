   -------------------------------------------------------------------------
         TESTING WIDGETS DEFINED IN *.UI FILES (SCREENSHOT COMPARISON)
   -------------------------------------------------------------------------
              This test is based on dev/qt/tests/auto/atwrapper/


HOW TO RUN THE TESTS

    Compile the necessary programs:
        qmake
        make

    Now there should be an executable 'uiloader'.
    Run the tests:
        ./uiloader



CONFIGURATION

    The tests can be executed on different machines. For every machine there
    is a config file <hostname>.ini.

    If this file is not available the tests won't be run on the machine.

        Example kayak.ini:

            [General]
            ftpBaseDir=/arthurtest
            ftpHost=wartburg
            ftpPass=anonymouspass
            ftpUser=anonymous
            output=testresults

            [engines]
            1\engine=uic
            size=1

    Don't modify the [engines] section. Later it will be used for styles.  The
    only thing you might want to modify is the ftpHost value. At the moment
    wartburg is just a server on which we test the tests.

    The actual test server is: kramer.troll.no



HOW IT WORKS

    There are 3 important folders:

        tst_uiloader/       Actual program executing the tests.
        tst_screenshot/     Program to generate one *.png file out of *.ui
        baseline/           This is a dir with *.ui files.
        
    ./uiloader (compiled from tst_uiloader) will use the tst_screenshot program
    and the files in baseline/.

    In order to generate screenshots the *.ui files in baseline/ will be used.
    Therefore a folder testresults/ (specified in the config) will be
    created/used.

    Structure of testresults/:
        
        |-- testresults
        |   `-- i686 Linux g++-4 full-config    That's buildKey().
        |       `-- 4.5.0                       Used Qt version.
        |           |-- uic                     Here are the generated images.
        |           |-- uic.baseline            Baseline downloaded from server.
        |           |-- uic.diff                Diff Images of failed tests.
        |           `-- uic.failed              Generated images of failed tests.

    If there is no baseline at the server the results in uic/ will be uploaded
    as a new baseline (first run for your machine). So to create a new baseline
    on the server you just need to remove your folders there.

    On the server side there is a dir /arthurtest with following structure
    (it will be created if it's not there):

        |-- arthurtest
        |   `-- <your machines hostname>
        |       `-- <buildKey()>
        |           `-- <qt version>
        |               |-- uic.baseline    Baseline.
        |               |-- uic.diff        Here the local diffs will be uploaded.
        |               `-- uic.failed      Here the local failed files will be uploaded.

    After a test run the files in local uic.diff/ and uic.failed/ will be
    uploaded to the server.

    For every run the uic.diff/ and uic.failed/ on the local and remote machine
    will be cleared in order to place new files there. On the local side uic/
    gets cleared, too.

