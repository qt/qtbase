1. Introduction

   This test suite is contributed by the testing team in the IBM Java Technology
   Center and used for the conformance test on the XML parsers based on XML 1.0
   Specification.
  
2. Test Suite Structure

   This XML conformance test suite consists of 149 valid tests, 51 invalid tests,
   and 746 not-well-formed tests. The configure files "ibm_oasis_valid.xml", 
   "ibm_oasis_invalid.xml", and "ibm_oasis_not-wf.xml" are located in a directory
   called "ibm". All test cases are in the directory tree starting from "ibm" 
   as shown below: 

                                      ibm                                 
                _______________________|_______________________
               |                       |                       |
             valid                  invalid                  not-wf
        _______|______           ______|_______          ______|_______
       |   |          |         |   |          |        |   |          |      
      P01 P02 ...... P89       P28 P29 ...... P76      P01 P02 ...... P89     
     __|__
    |     |
   out  ibm01v01.xml           ......
    |
   ibm01v01.xml 

3. File Naming Style

   The naming for a XML test cases follows the general form ibmXXYZZ.xml where
   XX is the number of XML production to be tested, Y is the character which 
   indicates the test type (v: valid, i: invalid, n: not-wf), ZZ is the test 
   case order number for the same XML production. For instance, ibm85n98.xml 
   means that it is an IBM not-well-formed test case number 98 for testing XML 
   production 85.

4. Test Coverage

   The XML test cases are designed based on the test patterns created according
   to the syntax rules and the WFC/VC constraints specified in each XML 1.0 
   production. 

                            