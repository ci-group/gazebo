/*
 * Copyright 2011 Nate Koenig & Andrew Howard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <sys/stat.h>
#include "sdf/sdf.hh"

std::vector<std::string> params;

using namespace sdf;

/////////////////////////////////////////////////
void help()
{
  std::cout << "This tool provides information about SDF files.\n\n";
  std::cout << "gzsdf <command>\n\n";
  std::cout << "Commands:\n";
  std::cout << "    describe [SDF version]     Print the SDF format.\n";
  std::cout << "    convert [file]             "
            << "In place conversion to the latest format.\n";
  std::cout << "    doc [SDF version]          Print HTML SDF.\n";
  std::cout << "    check [file] [SDF version] Check the SDF format for the";
  std::cout << " given file.\n";
  std::cout << "    print [SDF verison]         Prints SDF, useful for ";
  std::cout << " debugging and as a conversion tool.\n\n";
}

bool file_exists(const std::string _filename)
{
  struct stat st;
  return stat(_filename.c_str(), &st) == 0;
}

/////////////////////////////////////////////////
int main(int argc, char** argv)
{
  // Get parameters from command line
  for (int i = 1; i < argc; i++)
  {
    std::string p = argv[i];
    boost::trim(p);
    params.push_back(p);
  }

  if (params.empty() || params[0] == "help" || params[0] == "h")
  {
    help();
    return 0;
  }

  if ((params[0] == "check" || params[0] == "print" || params[0] == "convert"))
  {
    if (params.size() == 3)
      SDF::version = params[2];
  }
  else if (params.size() == 2)
    SDF::version = params[1];

  boost::shared_ptr<SDF> sdf(new SDF());
  if (!init(sdf))
  {
    std::cerr << "ERROR: SDF parsing the xml failed" << std::endl;
    return -1;
  }

  if (params[0] == "check")
  {
    if (params.size() < 2)
    {
      help();
      std::cerr << "Error: Expecting an xml file to parse\n\n";
      return -1;
    }

    if (!file_exists(params[1]))
      std::cerr << "Error: File doesn't exist[" << params[1] << "]\n";

    if (!readFile(params[1], sdf))
    {
      std::cerr << "Error: SDF parsing the xml failed\n";
      return -1;
    }
    std::cout << "Check complete\n";
  }
  else if (params[0] == "describe")
  {
    sdf->PrintDescription();
  }
  else if (params[0] == "doc")
  {
    sdf->PrintDoc();
  }
  else if (params[0] == "convert")
  {
    if (params.size() < 2)
    {
      help();
      std::cerr << "Error: Missing SDF file to convert\n\n";
      return -1;
    }

    if (!file_exists(params[1]))
      std::cerr << "Error: File doesn't exist[" << params[1] << "]\n";

    TiXmlDocument xmlDoc;
    if (xmlDoc.LoadFile(params[1]))
    {
      TiXmlElement *gazeboNode = xmlDoc.FirstChildElement("gazebo");
      sdf::Converter::Convert(gazeboNode, SDF::version, true);
      xmlDoc.SaveFile(params[1]);
    }
    else
      std::cerr << "Unable to load file[" << params[1] << "]\n";
  }
  else if (params[0] == "print")
  {
    if (params.size() < 2)
    {
      help();
      std::cerr << "Error: Expecting an xml file to parse\n\n";
      return -1;
    }

    if (!file_exists(params[1]))
      std::cerr << "Error: File doesn't exist[" << params[1] << "]\n";

    if (!readFile(params[1], sdf))
    {
      std::cerr << "Error: SDF parsing the xml failed\n";
      return -1;
    }
    sdf->PrintValues();
  }
  else
  {
    help();
    std::cerr << "Error: Unknown option[" << params[0] << "]\n";
  }

  std::cout << "Success\n";
  return 0;
}
