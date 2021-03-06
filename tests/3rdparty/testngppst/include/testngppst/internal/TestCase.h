/**
    TestNG++ is a practical, easy-to-use C/C++ xUnit framework.
    Copyright (C) <2009>  <Arthur Yuan: arthur.ii.yuan@gmail.com>

    TestNG++ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    TestNG++ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with TestNG++.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef __TESTNGPPST_TEST_CASE_H
#define __TESTNGPPST_TEST_CASE_H

#include <testngppst/testngppst.h>
#include <string>

#include <testngppst/internal/TestCaseInfoReader.h>
#include <testngppst/TestFixture.h>
#include <testngppst/runner/loaders/ModuleLoader.h>

TESTNGPPST_NS_START

struct TestCase
   : public TestCaseInfoReader
{
	TestCase( const std::string& nameOfCase
           , const std::string& nameOfFixture
		     , const std::string& nameOfSuite
           , TESTNGPPST_NS::TestCase* testcase
           , const std::string& file
           , unsigned int line)
	  : name(nameOfCase)
      , fixtureName(nameOfFixture)
	  , suiteName(nameOfSuite)
      , depends(testcase)
      , fileName(file)
      , lineOfFile(line)
   {}

	virtual ~TestCase() { }

	const std::string& getName() const
	{ return name; }

	const std::string& getNameOfFixture() const
	{ return fixtureName; }

	const std::string& getNameOfSuite() const
	{ return suiteName; }
	
    TESTNGPPST_NS::TestCase* 
    getDependedTestCase() const
    { return depends; }

	const std::string& getFileName() const
	{ return fileName; }

	const unsigned int getLineOfFile() const
	{ return lineOfFile; }

   virtual TESTNGPPST_NS::TestFixture* getFixture() const = 0;
   virtual void setFixture(TESTNGPPST_NS::TestFixture* fixture = 0) = 0;

   void setUp()
   {
       getFixture()->startMemChecker();
       getFixture()->setUp();
   }

   void tearDown()
   {
      TestFixture * fixture = getFixture();
      fixture->tearDown(); 
      getFixture()->verifyMemChecker();   
   }

   void run()
   {
      runDependedTestCase();
      runTest();
   }

   virtual void runTest() = 0;

   unsigned int numberOfTags() const { return 0; }

   const char** getTags() const
   {
     static const char* tags[1] = {0};
     return tags;
   }
   
   virtual const char* getMemCheckSwitch() const
   {
     static const char* memCheckSwitch = "none";
     return memCheckSwitch;
   }

   void setModuleLoader(ModuleLoader* _loader)
   {
       loader = _loader;
   }

   ModuleLoader *getLoader()
   {
       return loader;
   }

private:

   void runDependedTestCase()
   {
      if(depends == 0)
         return;

      depends->setFixture(getFixture());
      depends->run();
   }

private:
	std::string name;
	std::string fixtureName;
	std::string suiteName;
   TESTNGPPST_NS::TestCase* depends;
   std::string fileName;
	unsigned int lineOfFile;
	ModuleLoader* loader;
};

TESTNGPPST_NS_END

#endif

