#include "Save.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>

struct StrictObject {
	static char parseSymbol(std::fstream &file, char required = -1) {
		char token;
		file >> token;

		if (required != -1 && token != required) {
			std::cerr << "Invalid symbol encountered at index " << file.tellp() << ": ";
			std::cerr << "Expected to see '" << required << "' but got '" << token << "'" << std::endl;
			return -1;
		}

		return token;
	}

	static char parseString(std::fstream &file, int & dest, std::vector<std::string>& stringpool) {
		char token;
		int num;

		switch (token = parseSymbol(file)) {
			case 'y': {
				file >> num;
				std::string tmp;
				tmp.resize(num);
				parseSymbol(file, ':');
				file.get(&tmp[0], num + 1, -1);
				stringpool.push_back(tmp);
				dest = stringpool.size() - 1;
			} break;
			case 'R':
				file >> num;
				dest = num;
				break;
			default:
				dest = -1;
		}

		return token;
	}

	struct Member {
		int name;
		Member(int name, std::fstream& file, std::vector<std::string>&stringpool) : name(name) {
			switch (parseSymbol(file)) {
			case 't':
				type = Boolean;
				*(data.boolVal = new bool) = true;
				break;
			case 'f':
				type = Boolean;
				*(data.boolVal = new bool) = false;
				break;
			case 'i':
				type = Integer;
				file >> *(data.intVal = new int);
				break;
			case 'z':
				type = Integer;
				*(data.intVal = new int) = 0;
				break;
			case 'd':
				type = Double;
				file >> *(data.doubleVal = new double);
				break;
			case 'n':
				type = String;
				*(data.stringVal = new int) = -1;
				break;
			case 'w': {
				type = StringPair;
				data.stringPairVal = new std::pair<int, int>;
				parseString(file, (data.stringPairVal)->first, stringpool);
				parseString(file, (data.stringPairVal)->second, stringpool);
				parseSymbol(file, ':');
				parseSymbol(file, '0');
			} break;
			case 'y': case 'R': {
				type = String;
				file.seekp(-1, std::ios_base::cur);
				parseString(file, *(data.stringVal = new int), stringpool);
			} break;
			case 'a': {
				char tmp = parseSymbol(file);
				if (tmp == 'o') {
					type = ObjArray;
					data.objArrayVal = new std::vector<StrictObject>;
					do {
						if (tmp == 'u') {
							int num; file >> num;
						} else {
							file.seekp(-1, std::ios_base::cur);
							data.objArrayVal->push_back(StrictObject(file, stringpool));
						}
						tmp = parseSymbol(file);
					} while (tmp == 'o' || tmp == 'u');
				}
				else if (tmp == 'i' || tmp == 'z') {
					type = IntArray;
					data.intArrayVal = new std::vector<int>;
					do {
						int num = 0;
						if (tmp == 'i') {
							file >> num;
						}
						data.intArrayVal->push_back(num);
						tmp = parseSymbol(file);
					} while (tmp == 'i' || tmp == 'z');
				}
				else if (tmp == 'h') {
					type = IntArray;
					data.intArrayVal = new std::vector<int>;
				}

			} break;
			case 'q': {
				type = IntArray;
				data.intArrayVal = new std::vector<int>;

				while (parseSymbol(file) == ':') {
					int id; char req;
					file >> id; // ID of the chest collected
					file >> req; // Will always be 't' for 'true'
					data.intArrayVal->push_back(id);
				}
			} break;
			case 'o': {
				type = Object;
				file.seekp(-1, std::ios_base::cur);
				data.objVal = new StrictObject(file, stringpool);
			} break;
			default:
				break;
			}
		}

		enum Type {
			Undefined,
			Boolean,
			Integer,
			ObjArray,
			Object,
			IntArray,
			StringPair,
			String,
			Double
		} type = Type::Undefined;

		union DataValue {
			bool *boolVal;
			int *intVal;
			unsigned *uintVal;
			double *doubleVal;

			int *stringVal;
			std::pair<int, int> *stringPairVal;

			std::vector<int> *intArrayVal;
			std::vector<StrictObject> *objArrayVal;

			StrictObject *objVal;
		} data;
	};

	std::vector<Member> members;
	std::vector<std::string>& stringpool;

	StrictObject(std::fstream &file, std::vector<std::string> & stringpool)
		: stringpool(stringpool) {
		parseSymbol(file, 'o');
		int tmpName;
		bool parsing = true;
		while (parsing) {
			switch (char c = parseString(file, tmpName, stringpool)) {
				case 'y': case 'R': {
					members.push_back(Member(tmpName, file, stringpool));
					break;
				}
				case 'g': {
					parsing = false;
				} break;

				default: {
					std::cout << "Unknown Symbol \"" << c << "\" at " << file.tellp() << std::endl;
					parsing = false;
				}
			}

		}
	}
	
	void print(std::ostream & os, int indent = 1) {
		os << "{" << std::endl;
		for (int i = 0; i < members.size(); ++i) {
			for (int j = indent; j > 0; j--)  os << "  ";

			os << "\"" << stringpool[members[i].name] << "\": ";
			switch (members[i].type) {
			case Member::Type::Boolean:
				os << (*members[i].data.boolVal ? "true" : "false");
				break;

			case Member::Type::Integer:
				os << *members[i].data.intVal;
				break;

			case Member::Type::Object:
				members[i].data.objVal->print(os, indent + 1);
				break;

			case Member::Type::String:
				if (*members[i].data.stringVal < 0)
					os << "null";
				else
					os << "\"" << stringpool[*members[i].data.stringVal] << "\"";
				break;

			case Member::Type::Double:
				os << *members[i].data.doubleVal;
				break;

			case Member::Type::StringPair:
				os << "{" << std::endl;
				for (int j = indent + 1; j > 0; j--)  os << "  ";
				os << "\"" << stringpool[members[i].data.stringPairVal->first] << "\": \""
					<< stringpool[members[i].data.stringPairVal->second] << "\"" << std::endl;
				for (int j = indent + 1; j > 0; j--)  os << "  ";
				os << "}";
				break;
			
			case Member::Type::ObjArray:
				if (!members[i].data.objArrayVal->size()) {
					os << "[]";
					break;
				}
				os << "[" << std::endl;
				for (int j = indent + 1; j > 0; j--)  os << "  ";
				for (int j = 0; j < members[i].data.objArrayVal->size(); ++j) {
					(*members[i].data.objArrayVal)[j].print(os, indent + 2);
					if (j < members[i].data.objArrayVal->size() - 1)
						os << ", ";
					else
						os << std::endl;
				}
				for (int j = indent; j > 0; j--)  os << "  ";
				os << "]";
				break;
			
			case Member::Type::IntArray:
				if (!members[i].data.intArrayVal->size()) {
					os << "[]";
					break;
				}
				os << "[" << std::endl;
				for (int j = 0; j < members[i].data.intArrayVal->size(); ++j) {
					for (int j = indent + 1; j > 0; j--)  os << "  ";
					os << (*members[i].data.intArrayVal)[j];
					if (j < members[i].data.intArrayVal->size() - 1)
						os << ", ";
					os << std::endl;
				}
				for (int j = indent; j > 0; j--)  os << "  ";
				os << "]";
				break;
			}
			if(i < members.size() - 1)
				os << ",";
			os << std::endl;
		}
		for (int j = indent - 1; j > 0; j--)  os << "  ";
		os << "}";
	}
};

bool Save::parse(const char * filename) {
	std::fstream file;
	file.open(filename, std::fstream::in);
	if (!file.is_open()) {
		std::cout << "File failed to open. Please check filepath. [" << filename << "]" << std::endl;
		return false;
	}
	
	// All savedata is contained in a root-level object
	std::vector<std::string> stringpool;
	StrictObject data(file, stringpool);
	file.close();

	std::ofstream output;
	std::string outputFilename(filename);
	if (outputFilename.substr(outputFilename.size() - 4).compare(".sav") != 0) {
		std::cout << "File output failed!";
		return false;
	}

	outputFilename = outputFilename.substr(0, outputFilename.size() - 4);
	outputFilename.append(".json");
	output.open(outputFilename.c_str());

	data.print(output);

	return true;
}