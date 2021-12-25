#include "Save.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <filesystem>
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
		unsigned long long int num;

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
		int name = -1;
		Member(int name, std::fstream& file, std::vector<std::string>&stringpool) : name(name) {
			char symbol = parseSymbol(file);
			switch (symbol) {
			case 't':
				type = Member::Type::Boolean;
				*(data.boolVal = new bool) = true;
				break;
			case 'f':
				type = Member::Type::Boolean;
				*(data.boolVal = new bool) = false;
				break;
			case 'i':
				type = Member::Type::Integer;
				file >> *(data.intVal = new int);
				break;
			case 'z':
				type = Member::Type::Integer;
				*(data.intVal = new int) = 0;
				break;
			case 'd':
				type = Member::Type::Double;
				file >> std::setprecision(19) >> *(data.doubleVal = new double);
				break;
			case 'n':
				type = Member::Type::String;
				*(data.stringVal = new int) = -1;
				break;
			case 'w': {
				type = Member::Type::StringPair;
				data.stringPairVal = new std::pair<int, int>;
				parseString(file, (data.stringPairVal)->first, stringpool);
				parseString(file, (data.stringPairVal)->second, stringpool);
				parseSymbol(file, ':');
				parseSymbol(file, '0');
			} break;
			case 'y': case 'R': {
				type = Member::Type::String;
				file.seekp(-1, std::ios_base::cur);
				parseString(file, *(data.stringVal = new int), stringpool);
			} break;
			case 'b': {
				/*type = Member::Type::ValuePairArray;
				data.valuePairArrayVal = new std::vector<std::pair<int, StrictObject::Member*>>;

				while (file.peek() != 'h' ) {
					std::pair<int, StrictObject::Member*> pair;
					parseString(file, pair.first, stringpool);
					int tmpName = 0;
					pair.second = new Member(tmpName, file, stringpool);
					data.valuePairArrayVal->push_back(pair);
				}*/

				type = Member::Type::Object;
				file.seekp(-1, std::ios_base::cur);
				data.objVal = new StrictObject(file, stringpool);
				//data.valuePairArrayVal = new std::vector<std::pair<int, StrictObject::Member*>>;

				/*while (file.peek() != 'h') {
					std::pair<int, StrictObject::Member*> pair;
					parseString(file, pair.first, stringpool);
					int tmpName = 0;
					pair.second = new Member(tmpName, file, stringpool);
					data.valuePairArrayVal->push_back(pair);
				}*/
				
				
			} break;
			case 'a': {
				char tmp = parseSymbol(file);
				if (tmp == 'o') {
					type = Member::Type::ObjArray;
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
					type = Member::Type::IntArray;
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
					type = Member::Type::IntArray;
					data.intArrayVal = new std::vector<int>;
				}
				else if (tmp == 'y' || tmp == 'R') {
					type = Member::Type::StringArray;
					data.intArrayVal = new std::vector<int>;
					do {
						int dest;
						file.seekp(-1, std::ios_base::cur);
						parseString(file, dest, stringpool);
						data.intArrayVal->push_back(dest);
						tmp = parseSymbol(file);
					} while (tmp == 'i' || tmp == 'z');
				}

			} break;
			case 'q': {
				type = Member::Type::IntArray;
				data.intArrayVal = new std::vector<int>;

				while (parseSymbol(file) == ':') {
					int id; char req;
					file >> id; // ID of the chest collected
					file >> req; // Will always be 't' for 'true'
					data.intArrayVal->push_back(id);
				}
			} break;
			case 'o': {
				type = Member::Type::Object;
				file.seekp(-1, std::ios_base::cur);
				data.objVal = new StrictObject(file, stringpool);
			} break;
			default:
				break;
			}
		}

		enum class Type {
			Undefined,
			Boolean,
			Integer,
			ObjArray,
			Object,
			IntArray,
			StringPair,
			ValuePairArray,
			String,
			StringArray,
			Double
		} type = Member::Type::Undefined;

		union DataValue {
			bool* boolVal = nullptr;
			int* intVal;
			unsigned* uintVal;
			double* doubleVal;

			int* stringVal;
			std::pair<int, int>* stringPairVal;
			std::vector<std::pair<int, StrictObject::Member*>>* valuePairArrayVal;

			std::vector<int>* intArrayVal;
			std::vector<StrictObject>* objArrayVal;

			StrictObject* objVal;
		} data;

		void print(std::ostream& os, std::vector<std::string>& stringpool, int indent = 1) {
			if (name != -1) {
				os << "\"" << stringpool[name] << "\": ";
			}
			

			switch (type) {
			case Member::Type::Boolean:
				os << (*data.boolVal ? "true" : "false");
				break;

			case Member::Type::Integer:
				os << *data.intVal;
				break;

			case Member::Type::Object:
				data.objVal->print(os, indent + 1);
				break;

			case Member::Type::String:
				if (*data.stringVal < 0)
					os << "null";
				else
					os << "\"" << stringpool[*data.stringVal] << "\"";
				break;

			case Member::Type::Double:
				os << std::setprecision(19) << *data.doubleVal;
				break;

			case Member::Type::StringPair:
				os << "{" << std::endl;
				for (int j = indent + 1; j > 0; j--)  os << "  ";
				os << "\"" << stringpool[data.stringPairVal->first] << "\": \""
					<< stringpool[data.stringPairVal->second] << "\"" << std::endl;
				for (int j = indent + 1; j > 0; j--)  os << "  ";
				os << "}";
				break;
			case Member::Type::StringArray:
				if (!data.intArrayVal->size()) {
					os << "[]";
					break;
				}
				os << "[" << std::endl;
				for (int j = 0; j < data.intArrayVal->size(); ++j) {
					for (int j = indent + 1; j > 0; j--)  os << "  ";
					os << "\"" << stringpool[(*data.intArrayVal)[j]] << "\"";
					if (j < data.intArrayVal->size() - 1)
						os << ", ";
					os << std::endl;
				}
				for (int j = indent; j > 0; j--)  os << "  ";
				os << "]";
				break;
			case Member::Type::ValuePairArray:
				if (!data.valuePairArrayVal->size()) {
					os << "{}";
					break;
				}
				os << "{" << std::endl;
				for (int j = indent + 1; j > 0; j--)  os << "  ";
				for (int j = 0; j < data.valuePairArrayVal->size(); ++j) {
					os << "\"" << stringpool[(*data.valuePairArrayVal)[j].first] << "\": ";
					std::vector<std::pair<int, StrictObject::Member>>& arr = *(std::vector<std::pair<int, StrictObject::Member>>*)data.valuePairArrayVal;
					arr[j].second.print(os, stringpool, indent + 2);
					if (j < data.valuePairArrayVal->size() - 1)
						os << ", ";
					else
						os << std::endl;
				}
				for (int j = indent; j > 0; j--)  os << "  ";
				os << "}";
				break;
			case Member::Type::ObjArray:
				if (!data.objArrayVal->size()) {
					os << "[]";
					break;
				}
				os << "[" << std::endl;
				for (int j = indent + 1; j > 0; j--)  os << "  ";
				for (int j = 0; j < data.objArrayVal->size(); ++j) {
					(*data.objArrayVal)[j].print(os, indent + 2);
					if (j < data.objArrayVal->size() - 1)
						os << ", ";
					else
						os << std::endl;
				}
				for (int j = indent; j > 0; j--)  os << "  ";
				os << "]";
				break;

			case Member::Type::IntArray:
				if (!data.intArrayVal->size()) {
					os << "[]";
					break;
				}
				os << "[" << std::endl;
				for (int j = 0; j < data.intArrayVal->size(); ++j) {
					for (int j = indent + 1; j > 0; j--)  os << "  ";
					os << (*data.intArrayVal)[j];
					if (j < data.intArrayVal->size() - 1)
						os << ", ";
					os << std::endl;
				}
				for (int j = indent; j > 0; j--)  os << "  ";
				os << "]";
				break;
			}

			
		}
	};

	std::vector<Member> members;
	std::vector<std::string>& stringpool;

	StrictObject(std::fstream &file, std::vector<std::string> & stringpool)
		: stringpool(stringpool) {
		char c;
		file >> c;

		if (c == 'o' || c == 'c' || c == 'b') {
			if (c == 'c') {
				int tmpName;
				parseString(file, tmpName, stringpool);
			}
		}
		else {
			std::cerr << "Invalid symbol encountered at index " << file.tellp() << ": ";
			std::cerr << "Expected to see 'o' or 'c', but found '" << c << "'" << std::endl;
		}
		//char c = parseSymbol(file, 'o');
		int tmpName;
		bool parsing = true;

		while (parsing) {
			switch (char c = parseString(file, tmpName, stringpool)) {
				case 'y': case 'R': {
					members.push_back(Member(tmpName, file, stringpool));
					break;
				}

				case 'h': case 'g': {
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

			
			members[i].print(os, stringpool, indent);
			if (i < members.size() - 1)
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

	return "R" + std::to_string(indexOf + 1);
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
	else {
		std::ostringstream strs;
		strs << std::setprecision(15) << value;
		return "d" + strs.str();
	}

	
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
	std::string name_orig = name;
	name = toSaveString(name, stringpool);
	if (name == "y7:nchests") {
		std::string result = name + toSaveValue(file, stringpool, status);
		std::string nextName;
		file >> nextName;
		name = toSaveString(nextName, stringpool);
		return result + toSaveString(nextName, stringpool) + toSaveChestList(file, stringpool, status);
	}

	if (name_orig == "\"teamPersist\":" || name_orig == "\"doneRtc\":" || name_orig == "\"playerCharNames\":" || name_orig == "\"levelData\":") {
		std::string tmp;
		file >> tmp;
		std::string obj = toSaveObject(file, stringpool);
		obj[0] = 'b';
		obj[obj.size() - 1] = 'h';
		return name + obj;
	}

	if (name_orig == "\"flags\":" || name_orig == "\"globalFlags\":") {
		std::string result = name + "b";
		std::string tmp;
		file >> tmp;
		do {
			std::string tmp;
			file >> tmp;
			if (tmp[0] == '}') { break; }
			tmp = toSaveString(tmp, stringpool);
			std::string val;
			file >> val;
			char c = val[0];
			result += tmp + c;

		} while (true);

		return result + "h";
	}

	if (name_orig == "\"actives\":") {
		std::string token;
		file >> token;
		if (token.size() > 1) {
			return name + "qh";
		}

		std::string result = name + "q";
		do {
			std::string token;
			file >> token;
			if (token[0] == ']') {
				break;
			}

			if (token[token.size() - 1] == ',') {
				token = token.substr(0, token.size() - 1);
			}

			result += ":" + token + "t";

		} while (true);
		return result + "h";
	}

	if (name == "y1:x" || name == "y1:y" || name == "y4:life" || name == "y8:bahaNext") {

		std::streampos p = file.tellp();
		file.seekp(8, std::ios_base::beg);
		char c = file.peek();
		file.seekp(p);
		if (c != '2') {
			std::string value = toSaveValue(file, stringpool, status);
			if (value == "z") { value = "d0"; }
			else value[0] = 'd';
			return name + value;
		}
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
		file.seekp(9);
		std::string save = convertToSavefile(file);
		save = "oy4:datacy5:State" + save.substr(9, save.size() - 9);

		//std::string test = "n#" + sha1encode("n" + sha1encode("ns*al!t")).substr(4, 32);
		
		save += "#" + sha1encode(save + sha1encode(save + "s*al!t")).substr(4, 32);

		
		std::ofstream output;
		std::string outputFilename(filename);
		outputFilename = outputFilename.substr(0, outputFilename.size() - 5);
		outputFilename.append(".sav");
		
		if (std::filesystem::exists(outputFilename)) {
			// find available filename;
			int i = 0;
			std::string outputBackupName;
			do {
				outputBackupName = outputFilename + std::to_string(++i);
			} while (std::filesystem::exists(outputBackupName));
			std::cout << "Making backup of existing savefile: \""<< outputFilename << "\" -> \"" << outputBackupName << "\"" << std::endl;
			std::filesystem::copy(outputFilename, outputBackupName);
		}

		output.open(outputFilename.c_str());

		output << save;
	}
	else {
		// All savedata is contained in a root-level object

		std::vector<std::string> stringpool;

		// evoland1 format: 'oy4:datao' ... 'g' (string eval to game) 'w' (string eval to GameType) (string eval to Evo1) ':0' (string eval to time) 'd' (time)
		// evoland2 will start with 'oy4:datacy5:State'  ... 'g' (string eval to game) 'w' (string eval to GameType) (string eval to Evo2) ':0' (string eval to time) 'd' (time)
		file.seekp(8, std::ios_base::cur);
		stringpool.push_back("data");
		
		StrictObject data(file, stringpool);

		int dest1 = 0, dest2 = 0, dest3 = 0, dest4 = 0;
		double time;
		//file.seekp(-1, std::ios_base::cur);
		StrictObject::parseString(file, dest1, stringpool); // 'game'
		StrictObject::parseSymbol(file, 'w');
		StrictObject::parseString(file, dest2, stringpool); // 'GameType'
		StrictObject::parseString(file, dest3, stringpool); // game type
		StrictObject::parseSymbol(file, ':');
		StrictObject::parseSymbol(file, '0');
		StrictObject::parseString(file, dest4, stringpool); // 'time'
		StrictObject::parseSymbol(file, 'd');
		file >> std::setprecision(19) >> time;
		StrictObject::parseSymbol(file, 'g');

		std::ofstream output;
		std::string outputFilename(filename);
		outputFilename = outputFilename.substr(0, outputFilename.size() - 4);
		outputFilename.append(".json");
		output.open(outputFilename.c_str());
		output << "#evoland" << (stringpool[dest3])[3] << std::endl;
		output << "{" << std::endl;
		output << "  \"data\": ";
		data.print(output, 2);
		output << "," << std::endl;
		output << "  \"game\": {\n    \"GameType\": \"" << stringpool[dest3] << "\"\n  }," << std::endl;
		output << "  \"time\": " << std::setprecision(19) << time << std::endl;
		output << "}" << std::endl;
	}
	file.close();
	return true;
}