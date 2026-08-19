#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>


// OLED
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_RESET  -1
#define OLED_ADDR   0x3C

#define OLED_SDA 12
#define OLED_SCL 11

// 6 x 5 matrix keypad
//
//        C0    C1    C2    C3    C4
// R0    SHIFT   LN   nCr    (     )
// R1     x^n   sqrt  SIN   COS   TAN
// R2      7     8     9    DEL    AC
// R3      4     5     6     x     /
// R4      1     2     3     +     -
// R5      0     .    pi/e  x10^n   =

const uint8_t ROWS = 6;
const uint8_t COLS = 5;

uint8_t rowPins[ROWS] = {
  5, 4, 3, 2, 1, 0
};

uint8_t colPins[COLS] = {
  6, 7, 8, 9, 10
};

Adafruit_SSD1306 display(
  OLED_WIDTH,
  OLED_HEIGHT,
  &Wire,
  OLED_RESET
);

// Normal keys
const char* normalKeys[ROWS][COLS] = {

  // R0
  {
    "SHIFT", "LOG", "FACT", "(", ")"
  },

  // R1
  {
    "^", "SQRT", "SIN", "COS", "TAN"
  },

  // R2
  {
    "7", "8", "9", "DEL", "AC"
  },

  // R3
  {
    "4", "5", "6", "*", "/"
  },

  // R4
  {
    "1", "2", "3", "+", "-"
  },

  // R5
  {
    "0", ".", "PI", "EXP", "="
  }
};

// SHIFT keys
const char* shiftedKeys[ROWS][COLS] = {

  // R0
  {
    "SHIFT", "LN", "NCR", "(", ")"
  },

  // R1
  {
    "^", "SQRT", "ASIN", "ACOS", "ATAN"
  },

  // R2
  {
    "7", "8", "9", "DEL", "AC"
  },

  // R3
  {
    "4", "5", "6", "*", "/"
  },

  // R4
  {
    "1", "2", "3", "+", "-"
  },

  // R5
  {
    "0", ".", "E", "ANS", "="
  }
};

String expression = "";

double answer = 0.0;

bool hasAnswer = false;

bool shiftActive = false;

bool errorState = false;

// false = degrees
// true  = radians
bool radiansMode = false;

String scanKeypad()
{
  static String lastKey = "";
  static unsigned long lastTime = 0;

  for (uint8_t r = 0; r < ROWS; r++)
  {
    // Set all rows HIGH
    for (uint8_t rr = 0; rr < ROWS; rr++)
    {
      digitalWrite(rowPins[rr], HIGH);
    }

    // Select current row
    digitalWrite(rowPins[r], LOW);

    delayMicroseconds(50);

    for (uint8_t c = 0; c < COLS; c++)
    {
      if (digitalRead(colPins[c]) == LOW)
      {
        String key;

        if (shiftActive)
        {
          key = shiftedKeys[r][c];
        }
        else
        {
          key = normalKeys[r][c];
        }

        // Debounce
        if (key == lastKey &&
            millis() - lastTime < 150)
        {
          return "";
        }

        lastKey = key;
        lastTime = millis();

        // Wait until key is released
        while (digitalRead(colPins[c]) == LOW)
        {
          delay(1);
        }

        return key;
      }
    }
  }

  return "";
}

void drawCalculator()
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  String shownExpression = expression;

  // Only show the end of a long expression
  if (shownExpression.length() > 21)
  {
    shownExpression =
      shownExpression.substring(
        shownExpression.length() - 21
      );
  }

  display.setCursor(0, 0);

  display.print("> ");
  display.println(shownExpression);

  // Separator
  display.drawLine(
    0, 11,
    127, 11,
    SSD1306_WHITE
  );

  // ----------------------------------------------------------
  // Answer
  // ----------------------------------------------------------

  display.setCursor(0, 16);

  display.print("ANS: ");

  if (hasAnswer)
  {
    String a = String(answer, 8);

    if (a.length() > 18)
    {
      a = a.substring(0, 18);
    }

    display.println(a);
  }
  else
  {
    display.println("--");
  }

  // ----------------------------------------------------------
  // Mode
  // ----------------------------------------------------------

  display.setCursor(0, 32);

  if (shiftActive)
  {
    display.print("SHIFT ");
  }
  else
  {
    display.print("      ");
  }

  if (radiansMode)
  {
    display.println("RAD");
  }
  else
  {
    display.println("DEG");
  }

  // ----------------------------------------------------------
  // Status
  // ----------------------------------------------------------

  display.setCursor(0, 48);

  if (errorState)
  {
    display.print("ERROR");
  }
  else
  {
    display.print("READY");
  }

  display.display();
}

bool validNumber(double x)
{
  return isfinite(x);
}

double factorial(double x)
{
  if (!validNumber(x))
  {
    return NAN;
  }

  if (x < 0)
  {
    return NAN;
  }

  // Only integer factorials
  if (fabs(x - round(x)) > 1e-10)
  {
    return NAN;
  }

  // Avoid overflow
  if (x > 170)
  {
    return NAN;
  }

  double result = 1.0;

  for (int i = 2; i <= (int)round(x); i++)
  {
    result *= i;
  }

  return result;
}

double combination(double n, double r)
{
  if (!validNumber(n) ||
      !validNumber(r))
  {
    return NAN;
  }

  if (n < 0 || r < 0)
  {
    return NAN;
  }

  if (fabs(n - round(n)) > 1e-10 ||
      fabs(r - round(r)) > 1e-10)
  {
    return NAN;
  }

  n = round(n);
  r = round(r);

  if (r > n)
  {
    return NAN;
  }

  // Use the smaller side
  if (r > n - r)
  {
    r = n - r;
  }

  double result = 1.0;

  for (int i = 1; i <= r; i++)
  {
    result *= (n - r + i);
    result /= i;

    if (!validNumber(result))
    {
      return NAN;
    }
  }

  return result;
}

double toRadians(double x)
{
  if (radiansMode)
  {
    return x;
  }

  return x * PI / 180.0;
}

double fromRadians(double x)
{
  if (radiansMode)
  {
    return x;
  }

  return x * 180.0 / PI;
}

// expression
//      |
//      +-- term (+/- term)*
//
// term
//      |
//      +-- nCr (* / nCr)*
//
// power
//      |
//      +-- unary (^ power)?
//
// unary
//      |
//      +-- + unary
//      +-- - unary
//      +-- function
//      +-- primary

class Parser
{
private:

  String s;

  int pos;

public:

  Parser(String input)
  {
    s = input;
    pos = 0;
  }

  void skipSpaces()
  {
    while (
      pos < s.length() &&
      s[pos] == ' '
    )
    {
      pos++;
    }
  }

  bool match(char c)
  {
    skipSpaces();

    if (
      pos < s.length() &&
      s[pos] == c
    )
    {
      pos++;
      return true;
    }

    return false;
  }

  bool startsWith(String x)
  {
    skipSpaces();

    if (
      pos + x.length() >
      s.length()
    )
    {
      return false;
    }

    return
      s.substring(
        pos,
        pos + x.length()
      ) == x;
  }

  double parse()
  {
    double value =
      parseExpression();

    skipSpaces();

    if (pos != s.length())
    {
      return NAN;
    }

    return value;
  }

private:

  double parseExpression()
  {
    double value = parseTerm();

    while (true)
    {
      skipSpaces();

      if (match('+'))
      {
        double b = parseTerm();

        if (!validNumber(b))
        {
          return NAN;
        }

        value += b;
      }
      else if (match('-'))
      {
        double b = parseTerm();

        if (!validNumber(b))
        {
          return NAN;
        }

        value -= b;
      }
      else
      {
        break;
      }
    }

    return value;
  }

  // ----------------------------------------------------------
  // Multiplication / division
  // ----------------------------------------------------------

  double parseTerm()
  {
    double value = parseNCR();

    while (true)
    {
      skipSpaces();

      if (match('*'))
      {
        double b = parseNCR();

        if (!validNumber(b))
        {
          return NAN;
        }

        value *= b;
      }
      else if (match('/'))
      {
        double b = parseNCR();

        if (!validNumber(b))
        {
          return NAN;
        }

        if (fabs(b) < 1e-15)
        {
          return NAN;
        }

        value /= b;
      }
      else
      {
        break;
      }
    }

    return value;
  }

  // ----------------------------------------------------------
  // nCr
  // ----------------------------------------------------------

  double parseNCR()
  {
    double value = parsePower();

    while (true)
    {
      skipSpaces();

      if (startsWith("nCr"))
      {
        pos += 3;

        double b = parsePower();

        if (!validNumber(b))
        {
          return NAN;
        }

        value =
          combination(value, b);
      }
      else
      {
        break;
      }
    }

    return value;
  }

  // ----------------------------------------------------------
  // Powers
  // ----------------------------------------------------------

  double parsePower()
  {
    double value = parseUnary();

    skipSpaces();

    if (match('^'))
    {
      double exponent =
        parsePower();

      if (!validNumber(exponent))
      {
        return NAN;
      }

      value =
        pow(value, exponent);
    }

    return value;
  }

  // ----------------------------------------------------------
  // Unary +/- 
  // ----------------------------------------------------------

  double parseUnary()
  {
    skipSpaces();

    if (match('+'))
    {
      return parseUnary();
    }

    if (match('-'))
    {
      return -parseUnary();
    }

    return parseFunctionOrPrimary();
  }

  // ----------------------------------------------------------
  // Functions
  // ----------------------------------------------------------

  double parseFunctionOrPrimary()
  {
    skipSpaces();

    // ========================================================
    // ASIN
    // ========================================================

    if (startsWith("asin("))
    {
      pos += 5;

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      if (x < -1.0 ||
          x > 1.0)
      {
        return NAN;
      }

      double result =
        asin(x);

      return fromRadians(result);
    }

    // ========================================================
    // ACOS
    // ========================================================

    if (startsWith("acos("))
    {
      pos += 5;

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      if (x < -1.0 ||
          x > 1.0)
      {
        return NAN;
      }

      double result =
        acos(x);

      return fromRadians(result);
    }

    // ========================================================
    // ATAN
    // ========================================================

    if (startsWith("atan("))
    {
      pos += 5;

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      double result =
        atan(x);

      return fromRadians(result);
    }

    // ========================================================
    // SIN
    // ========================================================

    if (startsWith("sin("))
    {
      pos += 4;

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      return sin(toRadians(x));
    }

    // ========================================================
    // COS
    // ========================================================

    if (startsWith("cos("))
    {
      pos += 4;

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      return cos(toRadians(x));
    }

    // ========================================================
    // TAN
    // ========================================================

    if (startsWith("tan("))
    {
      pos += 4;

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      double angle =
        toRadians(x);

      // Undefined at 90°, 270°, etc.
      if (fabs(cos(angle)) < 1e-12)
      {
        return NAN;
      }

      return tan(angle);
    }

    // ========================================================
    // SQRT
    // ========================================================

    if (startsWith("sqrt("))
    {
      pos += 5;

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      if (x < 0)
      {
        return NAN;
      }

      return sqrt(x);
    }

    // ========================================================
    // LN
    // ========================================================

    if (startsWith("ln("))
    {
      pos += 3;

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      if (x <= 0)
      {
        return NAN;
      }

      return log(x);
    }

    // ========================================================
    // LOG
    // ========================================================

    if (startsWith("log("))
    {
      pos += 4;

      double base =
        parseExpression();

      if (!match(','))
      {
        return NAN;
      }

      double x =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      if (
        base <= 0 ||
        base == 1 ||
        x <= 0
      )
      {
        return NAN;
      }

      return log(x) / log(base);
    }

    // Otherwise primary
    return parsePrimary();
  }

  // ----------------------------------------------------------
  // Primary
  // ----------------------------------------------------------

  double parsePrimary()
  {
    skipSpaces();

    // Parentheses
    if (match('('))
    {
      double value =
        parseExpression();

      if (!match(')'))
      {
        return NAN;
      }

      return parsePostfix(value);
    }

    // PI
    if (startsWith("pi"))
    {
      pos += 2;

      return parsePostfix(PI);
    }

    // E
    if (startsWith("e"))
    {
      pos++;

      return parsePostfix(M_E);
    }

    // Number
    int start = pos;

    bool decimalFound = false;

    while (pos < s.length())
    {
      char c = s[pos];

      if (
        c >= '0' &&
        c <= '9'
      )
      {
        pos++;
      }
      else if (
        c == '.' &&
        !decimalFound
      )
      {
        decimalFound = true;
        pos++;
      }
      else
      {
        break;
      }
    }

    // Scientific notation
    if (
      pos < s.length() &&
      (
        s[pos] == 'e' ||
        s[pos] == 'E'
      )
    )
    {
      int epos = pos;

      pos++;

      if (
        pos < s.length() &&
        (
          s[pos] == '+' ||
          s[pos] == '-'
        )
      )
      {
        pos++;
      }

      int exponentStart = pos;

      while (
        pos < s.length() &&
        isDigit(s[pos])
      )
      {
        pos++;
      }

      // No exponent digits
      if (pos == exponentStart)
      {
        pos = epos;
      }
    }

    if (start == pos)
    {
      return NAN;
    }

    String num =
      s.substring(start, pos);

    double value =
      num.toDouble();

    return parsePostfix(value);
  }

  bool isDigit(char c)
  {
    return
      c >= '0' &&
      c <= '9';
  }

  // ----------------------------------------------------------
  // Factorial
  // ----------------------------------------------------------

  double parsePostfix(double value)
  {
    while (true)
    {
      skipSpaces();

      if (match('!'))
      {
        value =
          factorial(value);
      }
      else
      {
        break;
      }
    }

    return value;
  }
};

String convertExpression(String input)
{
  String output = "";

  for (int i = 0;
      i < input.length();)
  {
    // Inverse trig functions

    if (input.startsWith("ASIN", i))
    {
      output += "asin(";
      i += 4;
    }

    else if (input.startsWith("ACOS", i))
    {
      output += "acos(";
      i += 4;
    }

    else if (input.startsWith("ATAN", i))
    {
      output += "atan(";
      i += 4;
    }

    // --------------------------------------------------------
    // Normal trig
    // --------------------------------------------------------

    else if (input.startsWith("SIN", i))
    {
      output += "sin(";
      i += 3;
    }

    else if (input.startsWith("COS", i))
    {
      output += "cos(";
      i += 3;
    }

    else if (input.startsWith("TAN", i))
    {
      output += "tan(";
      i += 3;
    }

    // --------------------------------------------------------
    // Square root
    // --------------------------------------------------------

    else if (input.startsWith("SQRT", i))
    {
      output += "sqrt(";
      i += 4;
    }

    // --------------------------------------------------------
    // Natural log
    // --------------------------------------------------------

    else if (input.startsWith("LN", i))
    {
      output += "ln(";
      i += 2;
    }

    // --------------------------------------------------------
    // Log base
    // --------------------------------------------------------

    else if (input.startsWith("LOG", i))
    {
      output += "log(";
      i += 3;
    }

    // --------------------------------------------------------
    // nCr
    // --------------------------------------------------------

    else if (input.startsWith("NCR", i))
    {
      output += "nCr";
      i += 3;
    }

    // --------------------------------------------------------
    // Factorial
    // --------------------------------------------------------

    else if (input.startsWith("FACT", i))
    {
      output += "!";
      i += 4;
    }

    // --------------------------------------------------------
    // PI
    // --------------------------------------------------------

    else if (input.startsWith("PI", i))
    {
      output += "pi";
      i += 2;
    }

    // --------------------------------------------------------
    // Scientific notation
    // --------------------------------------------------------

    else if (input.startsWith("EXP", i))
    {
      output += "e";
      i += 3;
    }

    // --------------------------------------------------------
    // Answer
    // --------------------------------------------------------

    else if (input.startsWith("ANS", i))
    {
      if (hasAnswer)
      {
        output += String(answer, 10);
      }
      else
      {
        output += "0";
      }

      i += 3;
    }

    else
    {
      output += input[i];

      i++;
    }
  }

  return output;
}

String formatResult(double value)
{
  if (!validNumber(value))
  {
    return "ERROR";
  }

  // Use scientific notation for very large/small numbers
  double absoluteValue =
    fabs(value);

  String result;

  if (
    (absoluteValue >= 1e10) ||
    (
      absoluteValue > 0 &&
      absoluteValue < 1e-8
    )
  )
  {
    result = String(value, 6);
  }
  else
  {
    result = String(value, 10);
  }

  // Remove trailing zeroes
  if (result.indexOf('.') >= 0)
  {
    while (
      result.endsWith("0")
    )
    {
      result.remove(
        result.length() - 1
      );
    }

    if (result.endsWith("."))
    {
      result.remove(
        result.length() - 1
      );
    }
  }

  return result;
}

void calculate()
{
  if (expression.length() == 0)
  {
    return;
  }

  String converted =
    convertExpression(expression);

  Serial.print("Expression: ");
  Serial.println(expression);

  Serial.print("Converted: ");
  Serial.println(converted);

  Parser parser(converted);

  double result =
    parser.parse();

  if (!validNumber(result))
  {
    errorState = true;

    display.clearDisplay();

    display.setTextColor(
      SSD1306_WHITE
    );

    display.setTextSize(2);

    display.setCursor(0, 10);

    display.println("ERROR");

    display.setTextSize(1);

    display.setCursor(0, 38);

    display.println(
      "Invalid expression"
    );

    display.display();

    delay(1200);

    expression = "";

    errorState = false;

    drawCalculator();

    return;
  }

  answer = result;

  hasAnswer = true;

  expression =
    formatResult(result);

  drawCalculator();
}

void deleteLast()
{
  if (expression.length() == 0)
  {
    return;
  }

  // Delete complete tokens
  const char* tokens[] = {

    "SQRT",
    "FACT",

    "ASIN",
    "ACOS",
    "ATAN",

    "SIN",
    "COS",
    "TAN",

    "SHIFT",
    "ANS",
    "EXP",
    "NCR",
    "LOG",
    "LN",
    "PI"
  };

  const uint8_t tokenCount =
    sizeof(tokens) /
    sizeof(tokens[0]);

  for (uint8_t i = 0;
        i < tokenCount;
        i++)
  {
    String token =
      tokens[i];

    if (expression.endsWith(token))
    {
      expression.remove(
        expression.length() -
        token.length()
      );

      return;
    }
  }

  expression.remove(
    expression.length() - 1
  );
}

void processKey(String key)
{
  if (key.length() == 0)
  {
    return;
  }

  Serial.print("Key: ");
  Serial.println(key);

  if (key == "SHIFT")
  {
    shiftActive =
      !shiftActive;

    drawCalculator();

    return;
  }

  if (key == "AC")
  {
    expression = "";

    shiftActive = false;

    errorState = false;

    drawCalculator();

    return;
  }

  if (key == "DEL")
  {
    deleteLast();

    shiftActive = false;

    drawCalculator();

    return;
  }

  if (key == "=")
  {
    calculate();

    shiftActive = false;

    return;
  }

  if (key == "LOG")
  {
    expression += "LOG(";

    shiftActive = false;

    drawCalculator();

    return;
  }

  if (key == "FACT")
  {
    expression += "FACT";

    shiftActive = false;

    drawCalculator();

    return;
  }

  if (key == "ANS")
  {
    expression += "ANS";

    shiftActive = false;

    drawCalculator();

    return;
  }

  if (key == "E")
  {
    expression += "e";

    shiftActive = false;

    drawCalculator();

    return;
  }

  // Normal / shifted function keys
  //
  // SIN  -> SIN
  // SHIFT SIN -> ASIN
  //
  // COS  -> COS
  // SHIFT COS -> ACOS
  //
  // TAN  -> TAN
  // SHIFT TAN -> ATAN

  expression += key;

  // Most SHIFT functions are one-shot
  shiftActive = false;

  drawCalculator();
}


void setup()
{
  Serial.begin(115200);


  for (uint8_t r = 0;
       r < ROWS;
       r++)
  {
    pinMode(
      rowPins[r],
      OUTPUT
    );

    digitalWrite(
      rowPins[r],
      HIGH
    );
  }


  for (uint8_t c = 0;
       c < COLS;
       c++)
  {
    pinMode(
      colPins[c],
      INPUT_PULLUP
    );
  }


  Wire.setSDA(OLED_SDA);
  Wire.setSCL(OLED_SCL);

  Wire.begin();


  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDR
    )
  )
  {
    // OLED initialization failed
    while (true)
    {
      delay(100);
    }
  }


  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(2);

  display.setCursor(10, 8);

  display.println("PICO");

  display.setCursor(10, 32);

  display.println("CALC");

  display.display();

  delay(1000);

  drawCalculator();
}


void loop()
{
  String key =
    scanKeypad();

  if (key.length() > 0)
  {
    processKey(key);
  }

  delay(5);
}
