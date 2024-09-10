# wurl

Utility for fetching data from the web, in the form of a small C program that uses `libcurl`.

The idea is to be a good enough drop-in replacement for wget (not supporting absolutely every option that wget supports) to be usable at build-time, when packaging Linux distro packages.

Note that this utility is a bit experimental and a work in progress!

## Build

    make

## Test

    make test

## Install

    make PREFIX=/usr install

## General info

* Version: 0.0.1
* License: MIT
