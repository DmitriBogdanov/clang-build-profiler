// __________________________________ CONTENTS ___________________________________
// 
//    Modification script which adjusts relative links that would escape 
//    out of the 'docs/' folder so they go into the original repository
//    instead of returning 404.
// _______________________________________________________________________________

// Remove '<- to README.md' links, they are not necessary on the website
//
[...document.querySelectorAll("a[href='..'")]
    .forEach(el => {
        el.remove();
    });

// Adjust relative links that go beyond the doc files to link Github repo instead
//
// This way the the docs can be entirely functional as a local / repo files,  
// yet automatically work the way we would expect when deployed as a website
//
// The algorithm here is somewhat complicated in order to work with files in
// subdirectories (which may include several '../' without going "out" of the website)
//
function count_occurrences(str, substr) {
    return str.split(substr).length - 1
}

// Link replacement that works without 'navigation.instant' (which means relative links are preserved)
{
    const repo_link   = 'https://github.com/DmitriBogdanov/clang-build-profiler/blob/master/';
    
    const path_prefix   = '/UTL/';
    const path_relative = document.location.pathname.toString().substring(path_prefix.length);
    const path_level    = count_occurrences(path_relative, '/');
     
    [...document.querySelectorAll("a[href^='..'")] // ^= => "begins with" selector
        .forEach(el => {
            const href              = el.getAttribute('href')
            const href_return_level = count_occurrences(href, '..')
           
            if (href_return_level > path_level) {
                const ignore_size   = '../'.length * href_return_level
                const href_relative = el.getAttribute('href').substring(ignore_size) 
                el.setAttribute('href', repo_link + href_relative)
            }
        });
}
