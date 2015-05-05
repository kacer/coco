#!/bin/env python
# -*- coding: utf-8 -*-

import re
import argparse
import xml.etree.ElementTree as ET

from copy import deepcopy
from pprint import pprint
from collections import namedtuple

RE_INFO = r'\{(?P<ins>\d+),\s*(?P<outs>\d+),\s*(?P<rows>\d+),\s*(?P<cols>\d+),\s*(?P<b_ins>\d+),\s*(?P<b_outs>\d+),\s*(?P<funcs>\d+)\}'
RE_BLOCK = r'\(\[(?P<id>\d+)\] (?P<a>\d+), (?P<b>\d+), (?P<f>\d+)\)'

RE_COORDS = r'm (?P<x>[0-9.]+),(?P<y>[0-9.]+) (?P<w>[0-9.]+),(?P<h>[0-9.]+)'

FUNCTIONS = [
    ('c255', 0, u'255'),
    ('identity', 1, u'→A'),
    ('inversion', 1, u'INV'),
    ('b_or', 2, u'OR'),
    ('b_not1or2', 2, u'~A|B'),
    ('b_and', 2, u'AND'),
    ('b_nand', 2, u'NAND'),
    ('b_xor', 2, u'XOR'),
    ('rshift1', 1, u'>>1'),
    ('rshift2', 1, u'>>2'),
    ('swap', 2, u'SWP'),
    ('add', 2, u'+'),
    ('add_sat', 2, u'+S'),
    ('avg', 2, u'AVG'),
    ('max', 2, u'MAX'),
    ('min', 2, u'MIN'),
]


XML_NAMESPACES = {
    'inkscape': 'http://www.inkscape.org/namespaces/inkscape',
    'svg': 'http://www.w3.org/2000/svg',
}

CGP_INPUTS = 9

PRIMARY_IN_X = 31.803 + (4.130)
PRIMARY_IN_Y = 26.96
PRIMARY_IN_STEP = 26.14

PRIMARY_OUT_X = 723.204 - (4.130)
PRIMARY_OUT_Y = 135.36

FIRST_IN_X = 75.669
FIRST_IN_Y = 36.78 + (1.391 / 2)
FIRST_OUT_X = 108.053 + 9.850
FIRST_OUT_Y = 44.67 + (1.391 / 2)
INB_STEP = 15.778
BLOCK_STEP_X = 80
BLOCK_STEP_Y = 60

LINE_COLOR = '#00ab00'
INACTIVE_COLOR = '#cacaca'
ACTIVE_COLOR = '#000000'



def primary_in_coords(in_id):
    return PRIMARY_IN_X, PRIMARY_IN_Y + (in_id * PRIMARY_IN_STEP)

def block_pos(block_id):
    block_id -= CGP_INPUTS
    x = block_id / 4
    y = block_id % 4
    return x, y

def block_in_a_coords(block_id):
    bx, by = block_pos(block_id)
    ix = FIRST_IN_X + bx * BLOCK_STEP_X
    iy = FIRST_IN_Y + by * BLOCK_STEP_Y
    return ix, iy

def block_in_b_coords(block_id):
    ix, iy = block_in_a_coords(block_id)
    return ix, iy + INB_STEP

def block_out_coords(block_id):
    bx, by = block_pos(block_id)
    ix = FIRST_OUT_X + bx * BLOCK_STEP_X
    iy = FIRST_OUT_Y + by * BLOCK_STEP_Y
    return ix, iy

PATH_STYLE = u'fill:none;fill-rule:evenodd;stroke:{};stroke-width:1;stroke-linecap:butt;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1'
TEXT_STYLE = u'font-style:normal;font-weight:normal;font-size:7.66974545px;line-height:125%;font-family:sans-serif;text-align:center;letter-spacing:0px;word-spacing:0px;text-anchor:middle;fill:{};fill-opacity:1;stroke:none;stroke-width:1px;stroke-linecap:butt;stroke-linejoin:miter;stroke-opacity:1'

PRIMARYTXT_X = 17.89436
PRIMARYTXT_A_Y = 164.57831
PRIMARYTXT_B_Y = 192.57831

Block = namedtuple('Block', 'a,b,f')


def parse_chr(chr):
    cgp = {
        'blocks': {},
        'active_blocks': set(),
        'output': -1,
    }
    print chr

    initial = re.match(RE_INFO, chr, re.I)
    if not initial:
        raise RuntimeError('Invalid circuit')

    chr = chr[initial.end():]

    # load blocks
    for i in xrange(0, 32):
        block = re.match(RE_BLOCK, chr)
        chr = chr[block.end():]
        b_id = int(block.group('id'))
        b_a = int(block.group('a'))
        b_b = int(block.group('b'))
        b_f = int(block.group('f'))
        cgp['blocks'][b_id] = Block(b_a, b_b, b_f)

    # primary output
    output = re.match('\((\d+)\)', chr)
    cgp['output'] = int(output.group(1))

    # find active blocks
    cgp['active_blocks'].add(cgp['output'])
    for i in xrange(40, 9, -1):
        if i in cgp['active_blocks']:
            block = cgp['blocks'][i]
            if block.a >= CGP_INPUTS and FUNCTIONS[block.f][1] >= 1:
                cgp['active_blocks'].add(block.a)
            if block.b >= CGP_INPUTS and FUNCTIONS[block.f][1] >= 2:
                cgp['active_blocks'].add(block.b)

    return cgp


def svg_find_block(svg, block_id):
    return svg_find_by_label(svg, 'block' + str(block_id))


def svg_find_block_label(block):
    return svg_find_by_label(block, 'function')


def svg_find_by_label(parent, label):
    el = parent.findall(r".//*[@inkscape:label='{}']".format(label), XML_NAMESPACES)
    if not el:
        raise Exception('Element "{}" not found in svg template'.format(label))
    return el[0]


def svg_find_parent_by_label(parent, label):
    el = parent.findall(r".//*[@inkscape:label='{}']/..".format(label), XML_NAMESPACES)
    if not el:
        raise Exception('Element "{}" not found in svg template'.format(label))
    return el[0]


def svg_change_block_label(block, label):
    svg_label = svg_find_block_label(block)
    svg_label.findall('./{http://www.w3.org/2000/svg}tspan')[0].text = label


def svg_change_block_color(block, color):
    for el in block.iter():
        if el.get('style'):
            el.set('style', el.get('style').replace(u'#cacaca', color))

def svg_change_stroke_color(el, color):
    if el.get('style'):
        el.set('style', re.sub(r'stroke:(#[0-9a-f]+)', 'stroke:' + color, el.get('style')))

def svg_get_offset(block):
    tr = block.get('transform')
    if tr:
        match = re.match('translate\(([^,]+),([^,]+)\)', tr)
        if match:
            return float(match.group(1)), float(match.group(2))
    return 0, 0


def svg_connect(svg, left_id, right_id, right_block, input, color, primary_as_text=False):
    # LEFT
    if left_id >= CGP_INPUTS:
        lx, ly = block_out_coords(left_id)

    else:
        if primary_as_text and right_id >= 13:
            y = PRIMARYTXT_A_Y if input == 'a' else PRIMARYTXT_B_Y
            svg_text(right_block, PRIMARYTXT_X, y, unicode(left_id), color)
            return
        else:
            lx, ly = primary_in_coords(left_id)


    # RIGHT
    if input == 'a':
        rx, ry = block_in_a_coords(right_id)
    else:
        rx, ry = block_in_b_coords(right_id)

    svg_line(svg.getroot(), lx, ly, rx, ry, color)


def svg_line(el, lx, ly, rx, ry, color):
    ET.SubElement(el, '{http://www.w3.org/2000/svg}path', {
        'style': PATH_STYLE.format(color),
        'd': 'M {},{} L {},{}'.format(lx, ly, rx, ry),
    })


def svg_text(el, x, y, text, color):
    el = ET.SubElement(el, '{http://www.w3.org/2000/svg}text', {
        'style': TEXT_STYLE.format(color),
        'x': unicode(x),
        'y': unicode(y),
    })
    el.text = text


def fill_svg(cgp, svg, primary_as_text=False, primary_inactive_as_text=False):
    # primary output connection
    lx, ly = block_out_coords(cgp['output'])
    svg_line(svg.getroot(), lx, ly, PRIMARY_OUT_X, PRIMARY_OUT_Y, LINE_COLOR)

    # blocks
    for i, block in cgp['blocks'].items():
        active = i in cgp['active_blocks']
        svg_block = svg_find_block(svg, i)
        svg_change_block_label(svg_block, FUNCTIONS[block.f][2])

        if active:
            svg_change_block_color(svg_block, ACTIVE_COLOR)
            line_color = LINE_COLOR
            as_text = primary_as_text

        else:
            line_color = INACTIVE_COLOR
            as_text = primary_as_text or primary_inactive_as_text

        if FUNCTIONS[block.f][1] >= 1:
            svg_connect(svg, block.a, i, svg_block, 'a', line_color, as_text)
        else:
            tick = svg_find_by_label(svg_block, 'inA')
            svg_change_stroke_color(tick, INACTIVE_COLOR)

        if FUNCTIONS[block.f][1] >= 2:
            svg_connect(svg, block.b, i, svg_block, 'b', line_color, as_text)
        else:
            tick = svg_find_by_label(svg_block, 'inB')
            svg_change_stroke_color(tick, INACTIVE_COLOR)

    # put border back to the end
    mainrect = svg_find_by_label(svg, 'mainrect')
    parent = svg_find_parent_by_label(svg, 'mainrect')
    offset = svg_get_offset(parent)
    mainrect.set('x', unicode(offset[0] + float(mainrect.get('x'))))
    mainrect.set('y', unicode(offset[1] + float(mainrect.get('y'))))
    parent.remove(mainrect)
    svg.getroot().append(mainrect)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--circuit", "-c", required=True, help="CGP circuit (.chr)", type=str)
    parser.add_argument("--template", "-t", required=True, help="SVG template", type=str)
    parser.add_argument("--output", "-o", required=True, help="Output SVG file", type=str)
    parser.add_argument("--primary-as-text", "-xa", required=False, help="Output primary connections as text, not lines", action='store_true')
    parser.add_argument("--primary-inactive-as-text", "-xi", required=False, help="Output primary connections as text, not lines", action='store_true')
    args = parser.parse_args()

    for prefix, uri in XML_NAMESPACES.items():
        ET.register_namespace(prefix, uri)

    with open(args.circuit, 'r') as f:
        cgp = parse_chr(f.read())
    svg = ET.parse(args.template)
    fill_svg(cgp, svg, args.primary_as_text, args.primary_inactive_as_text)
    svg.write(args.output, encoding='UTF-8')


if __name__ == '__main__':
    main()

