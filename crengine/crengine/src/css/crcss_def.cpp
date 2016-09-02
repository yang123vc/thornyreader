#include "docformats.h"

const char* CRCSS_DEF = R"delimiter(
image {
  text-align: center;
  text-indent: 0px;
}

empty-line {
  height: 1em;
}

sub {
  vertical-align: sub;
  font-size: 70%;
}

sup {
  vertical-align: super;
  font-size: 70%;
}

body > image, section > image {
  text-align: center;
  margin-before: 1em;
  margin-after: 1em;
}

p > image {
  display: inline;
}

a {
  vertical-align: super;
  font-size: 80%;
}

p {
  margin-top: 0em;
  margin-bottom: 0em;
}

text-author {
  font-weight: bold;
  font-style: italic;
  margin-left: 5%;
}

empty-line {
  height: 1em;
}

epigraph {
  margin-left: 30%;
  margin-right: 4%;
  text-align: left;
  text-indent: 1px;
  font-style: italic;
  margin-top: 15px;
  margin-bottom: 25px;
  font-family: Times New Roman, serif;
}

strong, b {
  font-weight: bold;
}

emphasis, i {
  font-style: italic;
}

title {
  text-align: center;
  text-indent: 0px;
  font-size: 130%;
  font-weight: bold;
  margin-top: 10px;
  margin-bottom: 10px;
  font-family: Times New Roman, serif;
}

subtitle {
  text-align: center;
  text-indent: 0px;
  font-size: 150%;
  margin-top: 10px;
  margin-bottom: 10px;
}

title {
  page-break-before: always;
  page-break-inside: avoid;
  page-break-after: avoid;
}

body {
  text-align: justify;
  text-indent: 2em;
}

cite {
  margin-left: 30%;
  margin-right: 4%;
  text-align: justyfy;
  text-indent: 0px;
  margin-top: 20px;
  margin-bottom: 20px;
  font-family: Times New Roman, serif;
}

td, th {
  text-indent: 0px;
  font-size: 80%;
  margin-left: 2px;
  margin-right: 2px;
  margin-top: 2px;
  margin-bottom: 2px;
  text-align: left;
  padding: 5px;
}

th {
  font-weight: bold;
}

table > caption {
  padding: 5px;
  text-indent: 0px;
  font-size: 80%;
  font-weight: bold;
  text-align: left;
  background-color: #AAAAAA;
}

body[name="notes"] {
  font-size: 70%;
}

body[name="notes"]  section[id] {
  text-align: left;
}

body[name="notes"]  section[id] title {
  display: block;
  text-align: left;
  font-size: 110%;
  font-weight: bold;
  page-break-before: auto;
  page-break-inside: auto;
  page-break-after: auto;
}

body[name="notes"]  section[id] title p {
  text-align: left;
  display: inline;
}

body[name="notes"]  section[id] empty-line {
  display: inline;
}

code, pre {
  display: block;
  white-space: pre;
  font-family: "Courier New", monospace;
}
)delimiter";
