#include "rSimpleProgram.h"

using namespace rapio;

void
SimpleProgram::declareOptions(RAPIOOptions& o)
{
  // Declaring options is done first so that RAPIO can validate
  // options, generate help, etc. before your algorithm is ran.

  // Here we set the description of the program and the authors,
  // you should give credit to yourself for your hard work.
  o.setDescription("RAPIO based cool tool");
  o.setAuthors("Some awesome author of code");

  // Declaring options is done first so that RAPIO can validate
  // options, generate help, etc. before your algorithm is ran.

  // An optional string based param...default is "Test" if not set by user
  // Field, Default if not provided, Description
  o.optional("T", "Test", "Test option flag");

  // Boolean takes the field such as "x" or "toggleflag", and the description
  // defaults for boolean are false.  It's either provide or not.  So for example,
  // 'myalg -x' or 'myalg -toggleflag' or just 'myalg' with false.
  // Field, Description
  o.boolean("x", "Turn x on and off for some reason.");

  // A required parameter (program won't run without it).
  // Here there is no default since it's required
  // You give the field, a description and an example of usage:
  // Field, Description, Example
  // o.require("p", "pathname", "/tmp/example")
  // o.require("Z", "method1", "Set this to anything, it's just an example");
}

void
SimpleProgram::processOptions(RAPIOOptions& o)
{
  // Here is where you get/store validate options for use in your
  // algorithm.  Usually you can just put them in private fields
  // and use them later during execution.  Don't do any other startup
  // stuff here, do that in execute or processNewData (for algs)
  myTest = o.getString("T");
  // myTest = getFloat("T");
  // myTest = getInteger("T");
}

void
SimpleProgram::execute()
{
  LogInfo("Well this program is running, what to do?\n");
  LogInfo("Test was passed in as '" << myTest << "'\n");
}

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  SimpleProgram program = SimpleProgram();

  program.executeFromArgs(argc, argv);
}
