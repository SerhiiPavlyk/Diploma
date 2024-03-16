#pragma once

#include <boost/asio.hpp>
#include <iostream>


const std::string g_indexHTML = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Index Page</title>
</head>
<body>
    <h1>Welcome to the Index Page!</h1>
</body>
</html>
)";

const std::string g_homeHTML = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Home Page</title>
</head>
<body>
    <h1>Welcome to the Home Page!</h1>
    <form action="Calculator" method="post">
    <input type="text" name="field1" value="Value1">
    <input type="text" name="field2" value="Value2">
    <input type="submit" value="Submit">
    </form>
</body>
</html>
)";

const std::string g_homeRes = R"(
<!DOCTYPE html>
<html>
<body>
 <h1>Home Page</h1>
      <p>Value1 = field1</p>
      <p>Value2 = field2</p>
      <p>res = result</p>
<form action="/home.html" method="get">
  <button type="submit">Home</button>
</form>

</body>
</html>
)";

const std::string g_notFoundResponse = "HTTP/1.1 404 Not Found\r\n\r\nPage not found";