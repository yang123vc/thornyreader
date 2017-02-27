#include "docformats.h"

const char* CRCSS_EPUB = R"delimiter(

body, p {
  /* def.all */
  text-align: justify;
  text-indent: 1.2em;
  margin-top: 0em;
  margin-bottom: 0em;
  margin-left: 0em;
  margin-right: 0em;
}
.empty-line, empty-line {
  height: 1em;
}

h1, .title {
  font-size: 150%;
}

h2, .title1 {
  font-size: 140%;
}

h3, .title2 {
  font-size: 130%;
}

h4, .title3 {
  font-size: 120%;
}

h5, .title4 {
  font-size: 110%;
}

h1, h2, .title, .title1, .title2 {
  display: block;
  hyphenate: none;
  adobe-hyphenate: none;
  /* title.all */
  text-align: center;
  text-indent: 0em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 0em;
  margin-right: 0em;
  font-size: 110%;
  font-weight: bolder;
}

.subtitle {
  font-style: italic;
  margin-top: 0.5em;
  margin-bottom: 0.3em;
}

h3, h4, h5, h6, .title3, .title4, .title5, .subtitle {
  display: block;
  hyphenate: none;
  adobe-hyphenate: none;
  /* subtitle.all */
  text-align: center;
  text-indent: 0em;
  margin-top: 0.2em;
  margin-bottom: 0.2em;
  font-style: italic;
}

h1, h2, h3, .title, .title1, .title2, .title3 {
  page-break-before: always;
  page-break-inside: avoid;
  page-break-after: avoid;
}

.subtitle, h4, h5, h6, .title4, .title5 {
  page-break-inside: avoid;
  page-break-after: avoid;
}

img, image, .section_image, .coverpage {
  text-align: center;
  text-indent: 0px;
  display: block;
  margin: 6em;
}
p image, li image {
  display: inline;
}

*.justindent {
  text-align: justify;
  text-indent: 1.3em;
  margin-top: 0em;
  margin-bottom: 0em;
}

DocFragment {
  page-break-before: always;
}

a {
  display: inline;
  /* link.all */
  text-decoration: underline;
}

hr {
  height: 2px;
  background-color: #808080;
  margin-top: 0.5em;
  margin-bottom: 0.5em;
}

sub {
  vertical-align: sub;
  font-size: 70%;
}

sup {
  vertical-align: super;
  font-size: 70%;
}

ol {
  display: block;
  list-style-type: decimal;
  margin-left: 1em;
}
ul {
  display: block;
  list-style-type: disc;
  margin-left: 1em;
}
li {
  display: list-item;
  text-indent: 0em;
}

dl {
  margin-left: 0em;
}
dt {
  display: block;
  margin-left: 0em;
  margin-top: 0.3em;
  font-weight: bold;
}
dd {
  display: block;
  margin-left: 1.3em;
}

table {
  font-size: 80%;
}
td, th {
  text-indent: 0px;
  padding: 3px;
}
th {
  font-weight: bold;
  text-align: center;
  background-color: #DDD;
}
table caption {
  text-indent: 0px;
  padding: 4px;
  background-color: #EEE;
}

span {
  display: inline;
}

div {
  display: block;
}

.citation p {
  /* cite.all */
  text-align: justify;
  text-indent: 1.2em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 1em;
  margin-right: 1em;
  font-style: italic;
}

.epigraph p {
  /* epigraph.all */
  text-align: left;
  text-indent: 1.2em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 15%;
  margin-right: 1em;
  font-style: italic;
}

.v {
  text-align: left;
  text-align-last: right;
  text-indent: 1em hanging;
}

.stanza + .stanza {
  margin-top: 1em;
}

.stanza {
  /* poem.all */
  text-align: left;
  text-indent: 0em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 15%;
  margin-right: 1em;
  font-style: italic;
}

.poem {
  margin-top: 1em;
  margin-bottom: 1em;
  text-indent: 0px;
}

.epigraph_author {
  font-weight: bold;
  font-style: italic;
  margin-left: 15%;
}

.citation_author {
  font-weight: bold;
  font-style: italic;
  margin-left: 15%;
  margin-right: 10%;
}

.fb2_info {
  display: block;
  page-break-before: always;
}

b, strong, i, em, dfn, var, q, u, del, s, strike, small, big, sub, sup, acronym, tt, sa mp, kbd, code {
  display: inline;
}

b, strong {
  font-weight: bold;
}

i, em, dfn, var {
  font-style: italic;
}

u {
  text-decoration: underline;
}

del, s, strike, strikethrough {
  text-decoration: line-through;
}

small {
  font-size: 80%;
}

big {
  font-size: 130%;
}

pre {
  display: block;
  white-space: pre;
  /* pre.all */
  text-align: left;
  text-indent: 0em;
  margin-top: 0em;
  margin-bottom: 0em;
  margin-left: 0em;
  margin-right: 0em;
  font-family: "Courier New", "Droid Sans Mono",
  monospace;
}

.code, code {
  display: inline;
  /* pre.all */
  text-align: left;
  text-indent: 0em;
  margin-top: 0em;
  margin-bottom: 0em;
  margin-left: 0em;
  margin-right: 0em;
  font-family: "Courier New", "Droid Sans Mono",
  monospace;
}

nobr {
  display: inline;
  hyphenate: none;
  white-space: nowrap;
}

head, form, script {
  display: none;
}

)delimiter";