#include "WaveShieldIniFile.h"

#include <string.h>

const uint8_t WaveShieldIniFile::maxFilenameLen = INI_FILE_MAX_FILENAME_LEN;

WaveShieldIniFile::WaveShieldIniFile(FatReader& file, 
	  bool caseSensitive) : _file(file)
{
  _caseSensitive = caseSensitive;
}

WaveShieldIniFile::~WaveShieldIniFile()
{
  //if (_file)
  //  _file.close();
}


bool WaveShieldIniFile::validate(char* buffer, size_t len) 
{
  uint32_t pos = 0;
  error_t err;
  while ((err = readLine(buffer, len, pos)) == errorNoError)
    ;
  if (err == errorEndOfFile) {
    _error = errorNoError;
    return true;
  }
  else {
    _error = err;
    return false;
  }
}

bool WaveShieldIniFile::getValue(const char* section, const char* key,
			  char* buffer, size_t len, IniFileState &state) 
{
  bool done = false;
  
  switch (state.getValueState) {
  case IniFileState::funcUnset:
    state.getValueState = (section == NULL ? IniFileState::funcFindKey
			   : IniFileState::funcFindSection);
    state.readLinePosition = 0;
    break;
    
  case IniFileState::funcFindSection:
    if (findSection(section, buffer, len, state)) {
      if (_error != errorNoError)
	return true;
      state.getValueState = IniFileState::funcFindKey;
    }
    break;
    
  case IniFileState::funcFindKey:
    char *cp;
    if (findKey(section, key, buffer, len, &cp, state)) {
      if (_error != errorNoError)
	return true;
      // Found key line in correct section
      cp = skipWhiteSpace(cp);
      removeTrailingWhiteSpace(cp);

      // Copy from cp to buffer, but the strings overlap so strcpy is out
      while (*cp != '\0')
	*buffer++ = *cp++;
      *buffer = '\0';
      return true;
    }
    break;
    
  default:
    // How did this happen?
    _error = errorUnknownError;
    done = true;
    break;
  }
  
  return done;
}

bool WaveShieldIniFile::getValue(const char* section, const char* key,
			  char* buffer, size_t len) 
{
  IniFileState state;
  while (!getValue(section, key, buffer, len, state))
    ;
  return _error == errorNoError;
}


bool WaveShieldIniFile::getValue(const char* section, const char* key,
			 char* buffer, size_t len, char *value, size_t vlen) 
{
  if (getValue(section, key, buffer, len) < 0)
    return false; // error
  if (strlen(buffer) >= vlen)
    return false;
  strcpy(value, buffer);
  return true;
}


// For true accept: true, yes, 1
 // For false accept: false, no, 0
bool WaveShieldIniFile::getValue(const char* section, const char* key, 
			  char* buffer, size_t len, bool& val) 
{
  if (getValue(section, key, buffer, len) < 0)
    return false; // error
  
  if (strcasecmp(buffer, "true") == 0 ||
      strcasecmp(buffer, "yes") == 0 ||
      strcasecmp(buffer, "1") == 0) {
    val = true;
    return true;
  }
  if (strcasecmp(buffer, "false") == 0 ||
      strcasecmp(buffer, "no") == 0 ||
      strcasecmp(buffer, "0") == 0) {
    val = false;
    return true;
  }
  return false; // does not match any known strings      
}

bool WaveShieldIniFile::getValue(const char* section, const char* key,
			  char* buffer, size_t len, int& val) 
{
  if (getValue(section, key, buffer, len) < 0)
    return false; // error
  
  val = atoi(buffer);
  return true;
}

bool WaveShieldIniFile::getValue(const char* section, const char* key,	\
			  char* buffer, size_t len, uint16_t& val) 
{
  long longval;
  bool r = getValue(section, key, buffer, len, longval);
  if (r)
    val = uint16_t(longval);
  return r;
}

bool WaveShieldIniFile::getValue(const char* section, const char* key,
			  char* buffer, size_t len, long& val) 
{
  if (getValue(section, key, buffer, len) < 0)
    return false; // error
  
  val = atol(buffer);
  return true;
}

bool WaveShieldIniFile::getValue(const char* section, const char* key,
			  char* buffer, size_t len, unsigned long& val) 
{
  if (getValue(section, key, buffer, len) < 0)
    return false; // error

  char *endptr;
  unsigned long tmp = strtoul(buffer, &endptr, 10);
  if (endptr == buffer)
    return false; // no conversion
  if (*endptr == '\0') {
    val = tmp;
    return true; // valid conversion
  }
  // buffer has trailing non-numeric characters, and since the buffer
  // already had whitespace removed discard the entire results
  return false; 
}


bool WaveShieldIniFile::getValue(const char* section, const char* key,
			  char* buffer, size_t len, float & val) 
{
  if (getValue(section, key, buffer, len) < 0)
    return false; // error

  char *endptr;
  float tmp = strtod(buffer, &endptr);
  if (endptr == buffer)
    return false; // no conversion
  if (*endptr == '\0') {
    val = tmp;
    return true; // valid conversion
  }
  // buffer has trailing non-numeric characters, and since the buffer
  // already had whitespace removed discard the entire results
  return false; 
}



//int8_t IniFile::readLine(File &file, char *buffer, size_t len, uint32_t &pos)
WaveShieldIniFile::error_t WaveShieldIniFile::readLine(char *buffer, size_t len, uint32_t &pos)
{

  if (len < 3) 
    return errorBufferTooSmall;

  if (!_file.seekSet(pos))
    return errorSeekError;

  size_t bytesRead = _file.read(buffer, len);
  if (!bytesRead) {
    buffer[0] = '\0';
    //return 1; // done
    return errorEndOfFile;
  }
  
  for (size_t i = 0; i < bytesRead && i < len-1; ++i) {
    // Test for '\n' with optional '\r' too
    // if (endOfLineTest(buffer, len, i, '\n', '\r')
	
    if (buffer[i] == '\n' || buffer[i] == '\r') {
      char match = buffer[i];
      char otherNewline = (match == '\n' ? '\r' : '\n'); 
      // end of line, discard any trailing character of the other sort
      // of newline
      buffer[i] = '\0';
      
      if (buffer[i+1] == otherNewline)
	++i;
      pos += (i + 1); // skip past newline(s)
      //return (i+1 == bytesRead && !file.available());
      return errorNoError;
    }
  }

  
  buffer[len-1] = '\0'; // terminate the string
  return errorBufferTooSmall;
}

bool WaveShieldIniFile::isCommentChar(char c)
{
  return (c == ';' || c == '#');
}

char* WaveShieldIniFile::skipWhiteSpace(char* str)
{
  char *cp = str;
  while (isspace(*cp))
    ++cp;
  return cp;
}

void WaveShieldIniFile::removeTrailingWhiteSpace(char* str)
{
  char *cp = str + strlen(str) - 1;
  while (cp >= str && isspace(*cp))
    *cp-- = '\0';
}

bool WaveShieldIniFile::findSection(const char* section, char* buffer, size_t len, 
			     IniFileState &state) 
{
  if (section == NULL) {
    _error = errorSectionNotFound;
    return true;
  }

  error_t err = readLine( buffer, len, state.readLinePosition);
  
  if (err != errorNoError && err != errorEndOfFile) {
    // Signal to caller to stop looking and any error value
    _error = err;
    return true;
  }
    
  char *cp = skipWhiteSpace(buffer);
  //if (isCommentChar(*cp))
  //return (done ? errorSectionNotFound : 0);
  if (isCommentChar(*cp)) {
    // return (err == errorEndOfFile ? errorSectionNotFound : errorNoError);
    if (err == errorSectionNotFound) {
      _error = err;
      return true;
    }
    else
      return false; // Continue searching
  }
  
  if (*cp == '[') {
    // Start of section
    ++cp;
    cp = skipWhiteSpace(cp);
    char *ep = strchr(cp, ']');
    if (ep != NULL) {
      *ep = '\0'; // make ] be end of string
      removeTrailingWhiteSpace(cp);
      if (_caseSensitive) {
	if (strcmp(cp, section) == 0) {
	  _error = errorNoError;
	  return true;
	}
      }
      else {
	if (strcasecmp(cp, section) == 0) {
	  _error = errorNoError;
	  return true;
	}
      }
    }
  }
  
  // Not a valid section line
  //return (done ? errorSectionNotFound : 0);
  if (err == errorEndOfFile) {
    _error = errorSectionNotFound;
    return true;
  }
  
  return false;
}

// From the current file location look for the matching key. If
// section is non-NULL don't look in the next section
bool WaveShieldIniFile::findKey(const char* section, const char* key,
			 char* buffer, size_t len, char** keyptr,
			 IniFileState &state) 
{
  if (key == NULL || *key == '\0') {
    _error = errorKeyNotFound;
    return true;
  }

  error_t err = readLine( buffer, len, state.readLinePosition);
  if (err != errorNoError && err != errorEndOfFile) {
    _error = err;
    return true;
  }
  
  char *cp = skipWhiteSpace(buffer);
  // if (isCommentChar(*cp))
  //   return (done ? errorKeyNotFound : 0);
  if (isCommentChar(*cp)) {
    if (err == errorEndOfFile) {
      _error = errorKeyNotFound;
      return true;
    }
    else
      return false; // Continue searching
  }
  
  if (section && *cp == '[') {
    // Start of a new section
    _error = errorKeyNotFound;
    return true;
  }
  
  // Find '='
  char *ep = strchr(cp, '=');
  if (ep != NULL) {
    *ep = '\0'; // make = be the end of string
    removeTrailingWhiteSpace(cp);
    if (_caseSensitive) {
      if (strcmp(cp, key) == 0) {
	*keyptr = ep + 1;
	_error = errorNoError;
	return true;
      }
    }
    else {
      if (strcasecmp(cp, key) == 0) {
	*keyptr = ep + 1;
	_error = errorNoError;
	return true;
      }
    }
  }

  // Not the valid key line
  if (err == errorEndOfFile) {
    _error = errorKeyNotFound;
    return true;
  }
  return false;
}

bool WaveShieldIniFile::getCaseSensitive(void) 
{
  return _caseSensitive;
}

void WaveShieldIniFile::setCaseSensitive(bool cs)
{
  _caseSensitive = cs;
}

IniFileState::IniFileState()
{
  readLinePosition = 0;
  getValueState = funcUnset;
}
