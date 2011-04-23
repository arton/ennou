# coding: utf-8

require 'ennou.so'
require 'webrick/log'
require 'thread'

module Rack
  module Handler
    class Ennoumu

      @qname = 'EnnouMu_Queue'
      @nprocs = 2

      def self.config(options)
        set_option(:@qname, options[:qname])
        set_option(:@nprocs, options[:nprocs])
      end

      def self.set_option(v, s)
        instance_variable_set(v, s) if s
      end
        
      def self.run(app, options = {})
        @logger = options[:Logger] || ::WEBrick::Log::new
        script = ''
        if options[:config]
          if /^run\s+([^:]+)/ =~ IO::read(options[:config])
            script = $1.downcase
          end
        end
        port = options[:Port] || '80'
        host = (options[:Host] == '0.0.0.0') ? '+' : options[:Host]
        ::Ennou::Server.open(@qname, true) do |server|
          @server = server
          if server.controller?
            @stoprun = false
            @logger.info "Ennou(#{::Ennou::VERSION}) controller pid=#{$$} start"
            server.add "http://#{host}:#{port}/#{script}"
            pids = []
            1.upto(@nprocs) do
              pids << spawn('rackup -s Ennoumu', )
              @logger.info " spawn worker pid=#{pids.last}"
            end
            until @stoprun do
              sleep 1
            end  
            @logger.info "Ennou(#{::Ennou::VERSION}) controller pid=#{$$} stop"
          else
            @logger.info "Ennou(#{::Ennou::VERSION}) start for http://#{host}:#{port}/#{script} pid=#{$$}"
            loop do
              begin
                r = server.wait(60)
                next if r.nil?
                run_thread(app, *r)
              rescue Interrupt
                break
              end
            end
            @logger.info "Ennou(#{::Ennou::VERSION}) stop service for http://#{host}:#{port}/#{script} pid=#{$$}"
          end
        end
      end   

      def self.shutdown
        if @server.controller?
          @stoprun = true
        else
          @server.break
          @logger.info "going to shutdown ... pid=#{$$}"
        end
      end
      
      private
      
      def self.run_thread(app, env, io)
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
          rescue
            p $! if $debug
          ensure
            body.close if body.respond_to? :close
          end
        end
      end
    end
  end    
end
