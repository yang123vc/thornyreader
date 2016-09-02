#include "docformats.h"

const char* CRCSS_FB2 = R"delimiter(
body {
  text-align: left;
  margin: 0;
  text-indent: 0px;
}

p {
  /* def.all */ text-align: left; text-indent: 1.2em; margin-top: 0em; margin-bottom: 0em; margin-left: 0em; margin-right: 0em;
}

empty-line {
  height: 1em;
}

hr {
  height: 1px;
  background-color: #808080;
  margin-top: 0.5em;
  margin-bottom: 0.5em;
}

a {
  display: inline;
  /* link.all */
  text-decoration: underline;
}

a[type="note"] {
  /* footnote-link.all */
  font-size: 70%;
  vertical-align: super;
}

image {
  text-align: center;
  text-indent: 0px;
  display: block;
}

p image {
  display: inline;
}

li image {
  display: inline;
}

li {
  display: list-item;
  text-indent: 0em;
}

ul {
  display: block;
  list-style-type: disc;
  margin-left: 1em;
}

ol {
  display: block;
  list-style-type: decimal;
  margin-left: 1em;
}

v {
  text-align: left;
  text-align-last: right;
  text-indent: 1em hanging;
}

stanza {
  /* poem.all */ text-align: left; text-indent: 0em; margin-top: 0.3em; margin-bottom: 0.3em; margin-left: 10%; margin-right: 1em; font-style: italic;
}

stanza + stanza {
  margin-top: 1em;
}

poem {
  margin-top: 1em;
  margin-bottom: 1em;
  text-indent: 0px;
}

text-author {
  font-size: 70%;
  /* text-author.all */
  font-style: italic;
  font-weight: bolder;
  margin-left: 1em;
}

epigraph, epigraph p {
  /* epigraph.all */ text-align: left; text-indent: 1.2em; margin-top: 0.3em; margin-bottom: 0.3em; margin-left: 10%; margin-right: 1em; font-style: italic;
}

cite, cite p {
  /* cite.all */ text-align: justify; text-indent: 1.2em; margin-top: 0.3em; margin-bottom: 0.3em; margin-left: 1em; margin-right: 1em; font-style: italic;
}

title p, h1 p, h2 p {
  /* title.all */ text-align: center; text-indent: 0em; margin-top: 0.3em; margin-bottom: 0.3em; margin-left: 0em; margin-right: 0em; font-size: 110%; font-weight: bolder;
}

subtitle, subtitle p, h3 p, h4 p, h5 p, h6 p {
  /* subtitle.all */ text-align: center; text-indent: 0em; margin-top: 0.2em; margin-bottom: 0.2em; font-style: italic;
}

title, h1, h2, h3, h4, h5, h6, subtitle {
  hyphenate: none;
}

h1, h2, h3, h4, h5, h6 {
  display: block;
  margin-top: 0.5em;
  margin-bottom: 0.3em;
  padding: 10px;
  margin-top: 0.5em;
  margin-bottom: 0.5em;
}

title, h1, h2 {
  page-break-before: always;
  page-break-inside: avoid;
  page-break-after: avoid;
}

subtitle, h3, h4, h5, h6 {
  page-break-inside: avoid;
  page-break-after: avoid;
}

h1 {
  font-size: 150%;
}

h2 {
  font-size: 140%;
}

h3 {
  font-size: 130%;
}

h4 {
  font-size: 120%;
}

h5 {
  font-size: 110%;
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

table caption {
  text-indent: 0px;
  padding: 4px;
}

tt, samp, kbd, code, pre {
  font-family: "Courier New", "Courier", monospace;
}

code, pre {
  display: block;
  white-space: pre;
  /* pre.all */
  text-align: left;
  text-indent: 0em;
  margin-top: 0em;
  margin-bottom: 0em;
  margin-left: 0em;
  margin-right: 0em;
  font-family: "Courier New", "Droid Sans Mono", monospace;
}

body[name="notes"] {
  /* footnote.all */ font-size: 70%;
}

body[name="notes"] section title {
  display: run-in;
  text-align: left;
  $footnote-title.all page-break-before: auto;
  page-break-inside: auto;
  page-break-after: auto;
}

body[name="notes"] section title p {
  display: inline;
}

body[name="comments"] {
  /* footnote.all */ font-size: 70%;
}

body[name="comments"] section title {
  display: run-in;
  text-align: left;
  $footnote-title.all page-break-before: auto;
  page-break-inside: auto;
  page-break-after: auto;
}

body[name="comments"] section title p {
  display: inline;
}

description {
  display: block;
}

title-info {
  display: block;
}

annotation {
  /* annotation.all */ font-size: 90%; margin-left: 1em; margin-right: 1em; font-style: italic; text-align: justify; text-indent: 1.2em;
}

date {
  display: block;
  font-size: 80%;
  font-style: italic;
  text-align: center;
}

genre {
  display: none;
}

author {
  display: none;
}

book-title {
  display: none;
}

keywords {
  display: none;
}

lang {
  display: none;
}

src-lang {
  display: none;
}

translator {
  display: none;
}

document-info {
  display: none;
}

publish-info {
  display: none;
}

custom-info {
  display: none;
}

coverpage {
  display: none;
}

head, form, script {
  display: none;
}

b,strong,i,em,dfn,var,q,u,del,s,strike,small,big,sub,sup,acronym,tt,sa mp,kbd,code {
  display: inline;
}

sub {
  vertical-align: sub;
  font-size: 70%;
}

sup {
  vertical-align: super;
  font-size: 70%;
}

strong, b {
  font-weight: bold;
}

emphasis, i, em, dfn, var {
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

nobr {
  display: inline;
  hyphenate: none;
  white-space: nowrap;
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

img {
  margin: 0.5em;
  text-align: center;
  text-indent: 0em;
  border-style: solid;
  border-width: medium;
}
)delimiter";
