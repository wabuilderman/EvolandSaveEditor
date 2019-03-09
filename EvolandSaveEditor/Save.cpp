#include "Save.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include "TinySHA1.hpp"

struct StrictObject {
	// Create an empty Object
	StrictObject(std::vector<std::string> & stringpool) : stringpool(stringpool) {}

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
				file >> std::setprecision(19) >> *(data.doubleVal = new double);
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
							for (int i = 0; i < num; i++) {
								data.objArrayVal->push_back(StrictObject(stringpool));
							}
						}
						else if (tmp == 'n') {
							data.objArrayVal->push_back(StrictObject(stringpool));
						} else {
							file.seekp(-1, std::ios_base::cur);
							data.objArrayVal->push_back(StrictObject(file, stringpool));
						}
						tmp = parseSymbol(file);
					} while (tmp == 'o' || tmp == 'u' || tmp == 'n');
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
				os << std::setprecision(19) << *members[i].data.doubleVal;
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


std::string getFileExtension(const char * c) {
	if (!c) return "";
	int i = 0;
	while (++i,*c++);
	while (--i) if (*--c == '.') return c;
}

std::string toSaveMember(std::fstream & file, std::vector<std::string> & stringpool, int & status);

std::string toSaveString(std::string name, std::vector<std::string>& stringpool) {

	if (name.back() == ':')
		name = name.substr(0, name.size() - 1);

	name = name.substr(1, name.size() - 2);
	
	int indexOf = -1;
	for (int i = 0; i < stringpool.size(); ++i)
		if (stringpool[i] == name) indexOf = i;

	if (indexOf == -1) {
		stringpool.push_back(name);
		return "y" + std::to_string(name.size()) + ":" + name;
	}

	return "R" + std::to_string(indexOf);
}

std::string toSaveObject(std::fstream & file, std::vector<std::string> & stringpool) {
	std::string token = ",";
	std::string result;

	int numMembers = 0;
	int status = 0;

	std::string tmp;
	int pos = file.tellp();
	file >> tmp;

	if (tmp[0] == '}') {
		//file.seekp(-1, std::ios_base::cur);
		return "n";
	}
	else {
		file.seekp(pos);
	}

	while (token == ",") {
		numMembers++;
		status = 0;
		result += toSaveMember(file, stringpool, status);
		file.seekp(-1, std::ios_base::cur);
		file >> token;
	}

	file >> token;

	if (status == 1 && numMembers == 1) {
		return "w" + result + ":0";
	}
	else {
		return "o" + result + "g";
	}
}

std::string toSaveValue(std::fstream & file, std::vector<std::string> & stringpool, int & status) {
	std::string token;
	file >> token;

	if (token.back() == ',') {
		token = token.substr(0, token.size() - 1);
	}

	if (token[0] == '"') {
		status = 1;
		return toSaveString(token, stringpool);
	}
	if (token[0] == '[') {
		if (token.size() > 1 && token[1] == ']') {
			return "ah";
		}

		// convert array of values
		std::string subToken = ",";
		std::string result;

		int nCount = 0;
		
		while (subToken == ",") {
			int subStatus = 0;
			std::string value = toSaveValue(file, stringpool, subStatus);
			if (value == "n") nCount++; else {
				if (nCount > 1) {
					result = result.substr(0, result.size() - nCount) + "u" + std::to_string(nCount);
				}
				nCount = 0;
			}
			result += value;
			file.seekp(-1, std::ios_base::cur);
			file >> subToken;
			std::cout << subToken << std::endl;
		}
		file >> subToken;
		status = 3; // Indicate this was an Array
		return "a" + result + "h";
	}
	if (token[0] == '{') {
		status = 2; // Indicate this was an Object
		std::string result = toSaveObject(file, stringpool);
		return result;
	}
	if (token == "true")
		return "t";
	if (token == "false")
		return "f";
	if (token == "null")
		return "n";

	double value = atof(token.c_str());
	
	if (value == 0)
		return "z";

	if (value == (double)((int)(value))) {
		return "i" + std::to_string(((int)value));
	}

	std::ostringstream strs;
	strs << std::setprecision(15) << value;

	return "d" + strs.str();
}

std::string toSaveChestList(std::fstream & file, std::vector<std::string> & stringpool, int & status) {
	std::string result;
	std::string token;
	file >> token;
	token = ",";
	while (token == ",") {
		status = 0;
		std::string value = toSaveValue(file, stringpool, status);
		if (value == "z") value = "i0";
		result += ":" + value.substr(1, value.size() - 1)  + "t";
		file.seekp(-1, std::ios_base::cur);
		file >> token;
	}
	file >> token;
	return "q" + result + "h";
}

std::string toSaveMember(std::fstream & file, std::vector<std::string> & stringpool, int & status) {
	std::string name;
	file >> name;
	name = toSaveString(name, stringpool);
	if (name == "y7:nchests") {
		std::string result = name + toSaveValue(file, stringpool, status);
		std::string nextName;
		file >> nextName;
		name = toSaveString(nextName, stringpool);
		return result + toSaveString(nextName, stringpool) + toSaveChestList(file, stringpool, status);
	}

	if (name == "y1:x" || name == "y1:y" || name == "y4:life" || name == "y8:bahaNext") {
		std::string value = toSaveValue(file, stringpool, status);
		if (value == "z") { value = "d0"; }
		else value[0] = 'd';
		return name + value;
	}

	return name + toSaveValue(file, stringpool, status);
}

std::string convertToSavefile(std::fstream & file) {
	std::vector<std::string> stringpool;
	std::string result;
	char tokenC;
	file >> tokenC;
	return toSaveObject(file, stringpool);
}


std::string sha1encode(std::string data) {
	sha1::SHA1 s;
	uint32_t digest[5];
	char tmp[48];
	s.processBytes(data.c_str(), data.size());
	s.getDigest(digest);
	snprintf(tmp, 41, "%08x%08x%08x%08x%08x", digest[0], digest[1], digest[2], digest[3], digest[4]);
	return tmp;
}

bool Save::parse(const char * filename) {
	std::fstream file;
	file.open(filename, std::fstream::in);
	if (!file.is_open()) {
		std::cout << "File failed to open. Please check filepath. [" << filename << "]" << std::endl;
		return false;
	}
	std::string extension = getFileExtension(filename);
	if (extension == ".json") {
		std::cout << "Loaded JSON file" << std::endl;
		std::string save = convertToSavefile(file);

		//std::string test = "n#" + sha1encode("n" + sha1encode("ns*al!t")).substr(4, 32);
		
		save += "#" + sha1encode(save + sha1encode(save + "s*al!t")).substr(4, 32);

		
		std::ofstream output;
		std::string outputFilename(filename);
		outputFilename = outputFilename.substr(0, outputFilename.size() - 5);
		outputFilename.append(".sav");
		output.open(outputFilename.c_str());

		output << save;
	}
	else {
		// All savedata is contained in a root-level object
		std::vector<std::string> stringpool;
		StrictObject data(file, stringpool);

		std::ofstream output;
		std::string outputFilename(filename);
		outputFilename = outputFilename.substr(0, outputFilename.size() - 4);
		outputFilename.append(".json");
		output.open(outputFilename.c_str());
		data.print(output);
	}
	file.close();
	return true;
}