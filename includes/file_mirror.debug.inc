<?php

function file_mirror_backtrace()
{
    $output = "<div style='text-align: left; font-family: monospace;'>\n";
    $output .= "<b>Backtrace:</b><br />\n";
    $backtrace = debug_backtrace();

    foreach ($backtrace as $bt) {
        $args = '';
        foreach ($bt['args'] as $a) {
            if (!empty($args)) {
                $args .= ', ';
            }
            switch (gettype($a)) {
            case 'integer':
            case 'double':
                $args .= $a;
                break;
            case 'string':
                $a = htmlspecialchars(substr($a, 0, 64)).((strlen($a) > 64) ? '...' : '');
                $args .= "\"$a\"";
                break;
            case 'array':
                $args .= 'Array('.count($a).')';
                break;
            case 'object':
                $args .= 'Object('.get_class($a).')';
                break;
            case 'resource':
                $args .= 'Resource('.strstr($a, '#').')';
                break;
            case 'boolean':
                $args .= $a ? 'True' : 'False';
                break;
            case 'NULL':
                $args .= 'Null';
                break;
            default:
                $args .= 'Unknown';
            }
        }
        $output .= "<br />\n";
        $output .= "<b>file:</b> {$bt['line']} - {$bt['file']}<br />\n";
        $output .= "<b>call:</b> {$bt['class']}{$bt['type']}{$bt['function']}($args)<br />\n";
    }
    $output .= "</div>\n";
    return $output;
}
