# coding: utf-8

require 'ennou.so'

module Rack
  module Handler
    class Ennou

      QNAME = 'Ennou_Queue'

      def self.run(app, options = {})
        script = ''
        if options[:config]
          if /^run\s+([^:]+)/ =~ IO::read(options[:config])
            script = $1.downcase
          end
        end
        port = options[:Port] || '80'
        host = (options[:Host] == '0.0.0.0') ? '+' : options[:Host]
        @server = ::Ennou::Server.open(QNAME) do |server|
          server.add "http://#{host}:#{port}/#{script}"
          loop do
            r = server.wait(60)
            next if r.nil?
            env, io = *r
            Thread.start do
              env.update({'rack.version' => Rack::VERSION,
                          'rack.input' => io.input,
                          'rack.errors' => $stderr,
                          'rack.multithread' => true,
                          'rack.multiprocess' => false,
                          'rack.run_once' => false,
                          'rack.url_scheme' => env['URL_SCHEME']
                         })
              status, headers, body = app.call(env)
              begin
                io.status = status
                io.headers = headers
                body.each do |str|
                  io.write str
                end
                io.close
              ensure
                body.close if body.repond_to? :close
              end  
            end
          end
        end
      end   

      def self.shutdown
      end
    end
  end    
end
