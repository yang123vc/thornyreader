#include "include/crcss.h"

const char* CR_CSS = R"delimiter(
.empty-line, empty-line {
  display: block;
  height: 1em;
}

hr {
  display: block;
  height: 2px;
  background-color: #808080;
  margin-top: 0.5em;
  margin-bottom: 0.5em;
}

body, p, .justindent {
  display: block;
  text-align: justify;
  text-indent: 1.2em;
  margin-top: 0em;
  margin-bottom: 0em;
  margin-left: 0em;
  margin-right: 0em;
}

DocFragment {
  page-break-before: always;
}

/* TEST AREA */



/* TODO */


img, image, .section_image, .coverpage, coverpage {
  display: block;
  text-align: center;
  text-indent: 0px;
  margin-top: 0em;
  margin-bottom: 0em;
  margin-left: 0em;
  margin-right: 0em;
  padding-top: 1em;
  padding-bottom: 1em;
}

p image, li image {
  display: inline;
  padding-left: 1em;
  padding-right: 1em;
}

a, b, strong, q, u, del, s, strike, small, big, sub, sup, acronym, span, font {
  display: inline;
}
b, strong {
  font-weight: bold;
}
i, em, emphasis, dfn, var {
  display: inline;
  font-style: italic;
}
u {
  text-decoration: underline;
}
a {
  color: #0000EE;
}
del, s, strike, strikethrough {
  text-decoration: line-through;
}
small {
  font-size: 80%;
}
big {
  font-size: 120%;
}
sub {
  vertical-align: sub;
  font-size: 70%;
}
sup {
  vertical-align: super;
  font-size: 70%;
}
nobr {
  display: inline;
  hyphenate: none;
  white-space: nowrap;
}

h1, title, .title, .title0, .title1 {
  font-size: 140%;
}
h2, .title2 {
  font-size: 130%;
}
h3, .title3 {
  font-size: 130%;
}

h1, h2, h3, title,
h1 > p, h2 > p, h3 > p, title > p,
.title, .title0, .title1, .title2, .title3,
.title > p, .title0 > p, .title1 > p, .title2 > p, .title3 > p {
  display: block;
  page-break-before: always;
  page-break-inside: avoid;
  page-break-after: avoid;
  hyphenate: none;
  adobe-hyphenate: none;
  text-align: center;
  text-indent: 0em;
  margin-top: 0.4em;
  margin-bottom: 0.4em;
  margin-left: 0em;
  margin-right: 0em;
  font-weight: bold;
}
h4, h5, h6, subtitle,
h4 > p, h5 > p, h6 > p, subtitle > p,
.subtitle, .title4, .title5, .title6 {
  display: block;
  font-weight: bold;
  page-break-inside: avoid;
  page-break-after: avoid;
  hyphenate: none;
  adobe-hyphenate: none;
  font-size: 110%;
  text-align: center;
  text-indent: 0em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 0em;
  margin-right: 0em;
  font-style: italic;
}

pre, code, .code {
  display: block;
  white-space: pre;
  text-align: left;
  text-indent: 0em;
  margin-top: 0em;
  margin-bottom: 0em;
  margin-left: 0em;
  margin-right: 0em;
  font-family: "Droid Sans Mono", monospace;
}
tt, samp, kbd {
  display: inline;
  font-family: "Droid Sans Mono", monospace;
}

blockquote {
  display: block;
  margin-left: 1.5em;
  margin-right: 1.5em;
  margin-top: 0.5em;
  margin-bottom: 0.5em;
}
cite, .citation > p, cite > p {
  display: block;
  text-align: justify;
  text-indent: 1.2em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 1em;
  margin-right: 1em;
  font-style: italic;
}

ol, ul {
  display: block;
  margin-top: 1em;
  margin-bottom: 1em;
  margin-left: 0em;
  margin-right: 0em;
  padding-left: 40px;
}
ol {
  list-style-type: decimal;
}
ul {
  list-style-type: disc;
}
li {
  display: list-item;
  text-indent: 0em;
}

dl {
  display: block;
  margin-top: 1em;
  margin-bottom: 1em;
  margin-left: 0em;
  margin-right: 0em;
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
}
table > caption {
  text-indent: 0px;
  padding: 4px;
}

v, .v {
  text-align: left;
  text-align-last: right;
  text-indent: 1em hanging;
}

.stanza + .stanza, stanza + stanza {
  margin-top: 1em;
}

.stanza, stanza {
  text-align: left;
  text-indent: 0em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 15%;
  margin-right: 1em;
  font-style: italic;
}

.poem, poem {
  margin-top: 1em;
  margin-bottom: 1em;
  text-indent: 0px;
}

.epigraph > p, epigraph, epigraph > p {
  text-align: right;
  text-indent: 1em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 15%;
  margin-right: 1em;
  font-style: italic;
}

text-author, .epigraph_author, .citation_author {
  font-size: 80%;
  font-style: italic;
  text-align: right;
  margin-left: 15%;
  margin-right: 1em;
}

body[name="notes"], body[name="comments"] {
  font-size: 70%;
}

body[name="notes"] section title {
  display: run-in;
  text-align: left;
  page-break-before: auto;
  page-break-inside: auto;
  page-break-after: auto;
}
body[name="notes"] section title p {
  display: inline;
}

body[name="comments"] section title {
  display: run-in;
  text-align: left;
  page-break-before: auto;
  page-break-inside: auto;
  page-break-after: auto;
}
body[name="comments"] section title p {
  display: inline;
}

a[type="note"] {
  vertical-align: super;
  font-size: 70%;
  text-decoration: none;
}

annotation {
  display: block;
  font-size: 80%;
  margin-left: 1em;
  margin-right: 1em;
  font-style: italic;
  text-align: justify;
  text-indent: 1.2em;
}

.fb2_info {
  display: block;
  page-break-before: always;
}

description, title-info {
  display: block;
}

date {
  display: block;
  font-style: italic;
  text-align: center;
}

genre, author, book-title, keywords, lang, src-lang, translator {
  display: none;
}
document-info, publish-info, custom-info {
  display: none;
}

head, style, form, script {
  display: none;
}

)delimiter";